//------------------------------------------------------------------------------
//  actorcontext.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "PxConfig.h"
#include "physics/actorcontext.h"
#include "physics/physxstate.h"
#include "physics/utils.h"
#include "physics/geometry.h"
#include "physics/cooking.h"
#include "math/transform.h"

#define GET_ACTOR(i) ActorContext::actors[Ids::Index(i.id)]
#define GET_DYNAMIC(i) static_cast<PxRigidDynamic*>(ActorContext::actors[Ids::Index(i.id)].actor)
#define GET_RIGID(i) static_cast<PxRigidActor*>(ActorContext::actors[Ids::Index(i.id)].actor)
#define GET_SHAPE(i) ActorContext::shapes[Ids::Index(i.id)]

using namespace physx;

namespace Physics
{

//------------------------------------------------------------------------------
/**
    Kinematic is a rigid body flag, and the actors tracked here can be static, so the actor kind
    has to be checked before the flag is read.
*/
static bool
IsKinematic(PxRigidActor* actor)
{
    PxRigidBody* body = actor->is<PxRigidBody>();
    return body != nullptr && body->getRigidBodyFlags().isSet(PxRigidBodyFlag::eKINEMATIC);
}

Util::Array<Actor> ActorContext::actors;
Ids::IdGenerationPool ActorContext::actorPool;

Util::Array<Shape> ActorContext::shapes;
Ids::IdGenerationPool ActorContext::shapePool;

Util::Array<Aggregate> AggregateContext::aggregates;
Ids::IdGenerationPool AggregateContext::aggPool;

//------------------------------------------------------------------------------
/**
    FNV-1a hash
*/
static inline void
HashMix(uint64_t& hash, uint64_t value)
{
    for (int i = 0; i < 8; ++i)
    {
        hash ^= (value >> (i * 8)) & 0xFF;
        hash *= 1099511628211ULL;
    }
}

//------------------------------------------------------------------------------
/**
*/
static inline uint64_t
HashFloat(float value)
{
    uint32_t bits = *reinterpret_cast<uint32_t*>(&value);
    return (uint64_t)bits;
}

//------------------------------------------------------------------------------
/**
    Hash of the parts of a collider description that decide the cooked geometry.
    The transform is not hashed, because a shape can be moved without recooking, and
    neither is the material, which is applied without rebuilding the shape.
*/
static uint64_t
ColliderGeometryHash(const ColliderDescription& desc)
{
    uint64_t hash = 1469598103934665603ULL;

    // descriptions of different types should never compare equal on their parameters alone
    HashMix(hash, (uint64_t)desc.type);

    switch (desc.type)
    {
        case ColliderType_Sphere:
            HashMix(hash, HashFloat(desc.sphereRadius));
            break;
        case ColliderType_Box:
            HashMix(hash, HashFloat(desc.boxExtents.x));
            HashMix(hash, HashFloat(desc.boxExtents.y));
            HashMix(hash, HashFloat(desc.boxExtents.z));
            break;
        case ColliderType_Capsule:
            HashMix(hash, HashFloat(desc.capsuleRadius));
            HashMix(hash, HashFloat(desc.capsuleHalfHeight));
            break;
        case ColliderType_Plane:
            // a plane has no parameters, only the type matters
            break;
        case ColliderType_Mesh:
            HashMix(hash, desc.resource.StringHashCode());
            HashMix(hash, (uint64_t)desc.primGroup);
            HashMix(hash, (uint64_t)desc.topology);
            break;
        case ColliderType_HeightField:
            HashMix(hash, desc.resource.StringHashCode());
            HashMix(hash, HashFloat(desc.heightRange));
            HashMix(hash, HashFloat(desc.targetSizeX));
            HashMix(hash, HashFloat(desc.targetSizeY));
            break;
    }
    return hash;
}

//------------------------------------------------------------------------------
/**
*/
static bool
BuildGeometry(const ColliderDescription& desc, PxGeometryHolder& out)
{
    switch (desc.type)
    {
        case ColliderType_Sphere:
            out = PxSphereGeometry(desc.sphereRadius);
            return true;
        case ColliderType_Box:
            out = PxBoxGeometry(Neb2PxVec(desc.boxExtents));
            return true;
        case ColliderType_Capsule:
            out = PxCapsuleGeometry(desc.capsuleRadius, desc.capsuleHalfHeight);
            return true;
        case ColliderType_Plane:
            out = PxPlaneGeometry();
            return true;
        case ColliderType_Mesh:
        {
            const GeometryLoader& loader = GetGeometryLoader();
            if (loader.LoadMesh == nullptr)
            {
                n_error("BuildGeometry: no mesh geometry loader registered");
                return false;
            }
            MeshGeometryInput input;
            if (!loader.LoadMesh(desc.resource, desc.primGroup, input))
                return false;

            input.topology = desc.topology;
            return CookMesh(input, out);
        }
        case ColliderType_HeightField:
        {
            const GeometryLoader& loader = GetGeometryLoader();
            if (loader.LoadHeightField == nullptr)
            {
                n_error("BuildGeometry: no heightfield geometry loader registered");
                return false;
            }
            HeightFieldInput input;
            if (!loader.LoadHeightField(desc.resource, input))
                return false;

            return CookHeightField(input, desc.heightRange, desc.targetSizeX, desc.targetSizeY, out);
        }
    }
    return false;
}

//------------------------------------------------------------------------------
/**
*/
static PxMaterial*
ResolveShapeMaterial(IndexT materialId)
{
    if (materialId == InvalidIndex)
        return state.FallbackMaterial.material;
    return Physics::GetMaterial(materialId).material;
}

//------------------------------------------------------------------------------
/**
*/
static float
ResolveShapeDensity(IndexT materialId)
{
    if (materialId == InvalidIndex)
        return state.FallbackMaterial.density;
    return Physics::GetMaterial(materialId).density;
}

//------------------------------------------------------------------------------
/**
*/
static void
ApplyShapeFlags(PxShape* shape, bool isTrigger)
{
    if (isTrigger)
    {
        shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
        shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
    }
    else
    {
        shape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, false);
        shape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, true);
    }
}

//------------------------------------------------------------------------------
/**
    word0 is the collision group and word1 the feedback, which is what the simulation filter reads
*/
static void
ApplyShapeFilterData(PxShape* shape, ActorId actor)
{
    Actor& owner = ActorContext::GetActor(actor);
    PxFilterData filter;
    filter.word0 = owner.collisionGroup;
    filter.word1 = owner.feedback;
    shape->setSimulationFilterData(filter);
}

//------------------------------------------------------------------------------
/**
    Apply the material, flags and filter data of a description onto an existing shape
*/
static void
ApplyShapeState(Shape& shape, const ColliderDescription& desc)
{
    shape.materialId = desc.materialId;
    shape.isTrigger = desc.isTrigger;

    PxShape* pxShape = shape.shape;
    pxShape->setLocalPose(Neb2PxTrans(desc.transform));

    PxMaterial* material = ResolveShapeMaterial(desc.materialId);
    pxShape->setMaterials(&material, 1);
    ApplyShapeFlags(pxShape, desc.isTrigger);
    ApplyShapeFilterData(pxShape, shape.actor);
}

//------------------------------------------------------------------------------
/**
    Recompute the mass and inertia of a body. A mass of 0 means the mass is derived from the shape
    volumes and the material densities, otherwise the mass is distributed over the shapes.
*/
static void
ApplyMass(ActorId actor)
{
    Actor& owner = ActorContext::GetActor(actor);
    PxRigidActor* pxActor = ActorContext::GetPxActor(actor);
    if (!pxActor->is<PxRigidBody>())
        return;

    PxRigidBody* body = static_cast<PxRigidBody*>(pxActor);

    if (owner.mass > 0.0f)
    {
        PxRigidBodyExt::setMassAndUpdateInertia(*body, owner.mass);
        return;
    }

    SizeT num = pxActor->getNbShapes();
    if (num == 0)
        return;

    Util::StackArray<physx::PxShape*, ActorContext::DefaultShapeAlloc> buffer;
    PxShape** shapeBuffer = buffer.EmplaceArray(num);
    pxActor->getShapes(shapeBuffer, num);

    Util::StackArray<float, ActorContext::DefaultShapeAlloc> densities;
    float* densityBuffer = densities.EmplaceArray(num);
    SizeT numSimShapes = 0;
    for (SizeT i = 0; i < num; i++)
    {
        PxShape* pxShape = shapeBuffer[i];
        if (!(pxShape->getFlags() & PxShapeFlag::eSIMULATION_SHAPE))
            continue;

        float density;
        ShapeId sid((Ids::Id32)(uintptr_t)pxShape->userData);
        if (ActorContext::IsValidShape(sid) && ActorContext::GetShape(sid).shape == pxShape)
            density = ResolveShapeDensity(ActorContext::GetShape(sid).materialId);
        else
            density = ResolveShapeDensity(InvalidIndex);

        densityBuffer[numSimShapes++] = density;
    }

    if (numSimShapes > 0)
        PxRigidBodyExt::updateMassAndInertia(*body, densityBuffer, (PxU32)numSimShapes);
}

//------------------------------------------------------------------------------
/**
*/
ShapeId
ActorContext::AllocateShapeId(PxShape* pxShape, ActorId actor, uint64_t geometryHash, uint64_t userData)
{
    ShapeId id;
    bool const allocated = ActorContext::shapePool.Allocate(id.id);
    Ids::Id24 idx = Ids::Index(id.id);
    if (allocated)
    {
        ActorContext::shapes.Append(Physics::Shape());
        n_assert(idx == ActorContext::shapes.Size() - 1);
    }

    Shape& shape = ActorContext::shapes[idx];
    shape.id = id;
    shape.shape = pxShape;
    shape.actor = actor;
    shape.geometryHash = geometryHash;
    shape.userData = userData;
    pxShape->userData = (void*)(uintptr_t)id.id;

    Actor& owner = GET_ACTOR(actor);
    owner.shapes.Append(id);
    return id;
}

//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::AllocateActorId(PxRigidActor* pxActor)
{
    return ActorContext::AllocateActorId(pxActor, ActorResourceId::Invalid());
}

//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::AllocateActorId(PxRigidActor* pxActor, ActorResourceId res)
{
    ActorId id;
    bool const allocated = ActorContext::actorPool.Allocate(id.id);
    Ids::Id24 idx = Ids::Index(id.id);
    if (allocated)
    {
        ActorContext::actors.Append(Physics::Actor());
        n_assert(idx == ActorContext::actors.Size() - 1);
    }

    Actor& actor = ActorContext::actors[idx];
    actor.id = id;
    actor.actor = pxActor;
    actor.res = res;
    pxActor->userData = (void*)(uintptr_t)id.id;

    actor.mass = 0.0f;
    actor.collisionGroup = 0;
    actor.feedback = (uint32_t)CollisionFeedback_Full;
    actor.userData = 0;
    actor.shapes.Clear();
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void 
ActorContext::DiscardActor(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    Actor& actor = GET_ACTOR(id);
    PxScene* scene = actor.actor->getScene();
    if (scene != nullptr)
    {
        scene->removeActor(*actor.actor);
    }
    state.DiscardActor(id);
    actor.actor->release();

    // the actor owns its shapes, releasing it releases them as well
    for (IndexT i = 0; i < actor.shapes.Size(); i++)
    {
        ShapeId sid = actor.shapes[i];
        Shape& shape = GET_SHAPE(sid);
        shape.shape = nullptr;
        ActorContext::shapePool.Deallocate(sid.id);
    }
    actor.shapes.Clear();

    ActorContext::actorPool.Deallocate(id.id);
}

//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::CreateBox(Math::vector const& extends, IndexT materialId, ActorType type, Math::mat4 const & transform, IndexT sceneId)
{
    Scene & scene = Physics::GetScene(sceneId);

    PxRigidActor* newActor = state.CreateActor(type, transform);

    Material const & material = Physics::GetMaterial(materialId);
    PxShape* shape = PxRigidActorExt::createExclusiveShape(*newActor, PxBoxGeometry(Neb2PxVec(extends)), *material.material);
    if (type != ActorType::Static)
    {
        PxRigidBodyExt::updateMassAndInertia(*static_cast<PxRigidDynamic*>(newActor), material.density);
    }
    scene.scene->addActor(*newActor);
    return AllocateActorId(newActor);
}


//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::CreateSphere(float radius, IndexT materialId, ActorType type, Math::mat4 const & transform, IndexT sceneId)
{
    Scene& scene = Physics::GetScene(sceneId);

    PxRigidActor* newActor = state.CreateActor(type, transform);

    Material const& material = Physics::GetMaterial(materialId);
    PxShape* shape = PxRigidActorExt::createExclusiveShape(*newActor, PxSphereGeometry(radius), *material.material);
    if (type != ActorType::Static)
    {
        PxRigidBodyExt::updateMassAndInertia(*static_cast<PxRigidDynamic*>(newActor), material.density);
    }
    scene.scene->addActor(*newActor);
    return AllocateActorId(newActor);
}


//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::CreateCapsule(float radius, float halfHeight, IndexT materialId, ActorType type, Math::mat4 const & transform, IndexT sceneId)
{
    Scene& scene = Physics::GetScene(sceneId);

    PxRigidActor* newActor = state.CreateActor(type, transform);

    Material const& material = Physics::GetMaterial(materialId);
    PxShape* shape = PxRigidActorExt::createExclusiveShape(*newActor, PxCapsuleGeometry(radius, halfHeight), *material.material);
    if (type != ActorType::Static)
    {
        PxRigidBodyExt::updateMassAndInertia(*static_cast<PxRigidDynamic*>(newActor), material.density);
    }
    scene.scene->addActor(*newActor);
    return AllocateActorId(newActor);
}

//--------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::CreateConvexHull(float* vertices, int numVertices, IndexT materialId, ActorType type, Math::mat4 const& transform, IndexT sceneId)
{
    Scene& scene = Physics::GetScene(sceneId);

    PxRigidActor* newActor = state.CreateActor(type, transform);

    Material const& material = Physics::GetMaterial(materialId);

    PxConvexMeshDesc convexDesc;
    convexDesc.points.count  = numVertices / 3;
    convexDesc.points.stride = sizeof(float) * 3;
    convexDesc.points.data   = vertices;
    convexDesc.flags         = PxConvexFlag::eCOMPUTE_CONVEX | PxConvexFlag::eFAST_INERTIA_COMPUTATION;

    PxTolerancesScale scale;
    PxCookingParams cookingParams(scale);

#ifdef NEBULA_DEBUG
    // mesh should be validated before cooking without the mesh cleaning
    //bool convexMeshValid = PxValidateConvexMesh(cookingParams, convexDesc);
    //n_assert(convexMeshValid);
#endif

    PxDefaultMemoryOutputStream buf;
    if(!PxCookConvexMesh(cookingParams, convexDesc, buf))
        return NULL;

    PxConvexMesh* convexMesh = PxCreateConvexMesh(cookingParams, convexDesc, state.physics->getPhysicsInsertionCallback());

    PxShape* shape = PxRigidActorExt::createExclusiveShape(*newActor, PxConvexMeshGeometry(convexMesh), *material.material);

    if (type != ActorType::Static)
    {
        PxRigidBodyExt::updateMassAndInertia(*static_cast<PxRigidDynamic*>(newActor), material.density);
    }
    scene.scene->addActor(*newActor);
    return AllocateActorId(newActor);

}

//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::CreatePlane(Math::plane const& plane, IndexT materialId, IndexT sceneId)
{
    Scene & scene = Physics::GetScene(sceneId);

    PxPlane pxPlane(Neb2PxPnt(get_point(plane)), Neb2PxVec(get_normal(plane)));
    PxTransform transform = PxTransformFromPlaneEquation(pxPlane);

    PxRigidActor* newActor = scene.physics->createRigidStatic(transform);

    Material const & material = Physics::GetMaterial(materialId);

    PxShape * shape = PxRigidActorExt::createExclusiveShape(*newActor, PxPlaneGeometry(), *material.material);

    scene.scene->addActor(*newActor);
    return AllocateActorId(newActor);
}


//------------------------------------------------------------------------------
/**
*/
Actor&
ActorContext::GetActor(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    return ActorContext::actors[Ids::Index(id.id)];
}

//------------------------------------------------------------------------------
/**
*/
void ActorContext::SetTransform(ActorId id, Math::mat4 const & transform)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    PxRigidActor* actor = GET_RIGID(id);
    if (IsKinematic(actor))
    {
        static_cast<PxRigidDynamic*>(actor)->setKinematicTarget(Neb2PxTrans(transform));
    }
    else
    {
        actor->setGlobalPose(Neb2PxTrans(transform));
    }
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
ActorContext::GetTransform(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    return Px2NebMat(GET_RIGID(id)->getGlobalPose());
}

//------------------------------------------------------------------------------
/**
*/
void
ActorContext::SetPositionOrientation(ActorId id, Math::vec3 const& position, Math::quat const& orientation)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    PxRigidActor* actor = GET_RIGID(id);
    if (IsKinematic(actor))
    {
        static_cast<PxRigidDynamic*>(actor)->setKinematicTarget(Neb2PxTrans(position, orientation));
    }
    else
    {
        actor->setGlobalPose(Neb2PxTrans(position, orientation));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ActorContext::GetPositionOrientation(ActorId id, Math::vec3& pos, Math::quat& orientation)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    auto const pxTrans = GET_RIGID(id)->getGlobalPose();
    pos = Px2NebVec(pxTrans.p);
    orientation = Px2NebQuat(pxTrans.q);
}

//------------------------------------------------------------------------------
/**
*/
void
ActorContext::SetLinearVelocity(ActorId id, Math::vector speed)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    GET_DYNAMIC(id)->setLinearVelocity(Neb2PxVec(speed));
}

//------------------------------------------------------------------------------
/**
*/
Math::vector 
ActorContext::GetLinearVelocity(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    return Px2NebVec(GET_DYNAMIC(id)->getLinearVelocity());
}

//------------------------------------------------------------------------------
/**
*/
void
ActorContext::SetAngularVelocity(ActorId id, Math::vector speed)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    GET_DYNAMIC(id)->setAngularVelocity(Neb2PxVec(speed));
}

//------------------------------------------------------------------------------
/**
*/
Math::vector
ActorContext::GetAngularVelocity(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    return Px2NebVec(GET_DYNAMIC(id)->getAngularVelocity());
}

//------------------------------------------------------------------------------
/**
*/
void
ActorContext::ApplyImpulseAtPos(ActorId id, const Math::vector& impulse, const Math::point& pos)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    PxRigidDynamic* actor = GET_DYNAMIC(id);    
    PxRigidBodyExt::addForceAtPos(*actor, Neb2PxVec(impulse), Neb2PxPnt(pos));
}

//------------------------------------------------------------------------------
/**
*/
physx::PxRigidActor * 
ActorContext::GetPxActor(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    return static_cast<physx::PxRigidActor*>(GET_ACTOR(id).actor);
}

//------------------------------------------------------------------------------
/**
*/
physx::PxRigidDynamic* 
ActorContext::GetPxDynamic(ActorId id)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));
    return GET_DYNAMIC(id);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
ActorContext::GetShapeCount(ActorId id)
{
    if (id.id != Ids::InvalidId32)
    {
        physx::PxRigidActor* actor = GetPxActor(id);
        if (actor != nullptr)
        {
            return actor->getNbShapes();
        }
    }
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
void ActorContext::GetShapes(ActorId id, ShapeArrayType& shapes)
{
    if (id.id != Ids::InvalidId32)
    {
        physx::PxRigidActor* actor = GetPxActor(id);
        if (actor != nullptr)
        {
            auto count = actor->getNbShapes();

            Util::StackArray<physx::PxShape*, DefaultShapeAlloc> buffer;
            PxShape** shapeBuffer = buffer.EmplaceArray(count);
            ShapeHandle* handles = shapes.EmplaceArray(count);
            actor->getShapes(shapeBuffer, count);
            for (int i = 0; i < count; ++i)
            {
                handles[i].shape = shapeBuffer[i];
            }
        }
    }
}
Math::transform ActorContext::GetShapeTransform(const ShapeHandle& shape)
{
    if (shape.IsValid())
    {
        return Px2NebTrans(shape.shape->getLocalPose());
    }
    return Math::transform();
}

void ActorContext::SetShapeTransform(const ShapeHandle& shape, const Math::transform& transform)
{
    if (shape.IsValid())
    {
        shape.shape->setLocalPose(Neb2PxTrans(transform));
    }
}

//------------------------------------------------------------------------------
/**
*/
void ActorContext::SetCollisionFeedback(ActorId id, CollisionFeedback feedback)
{
    if (id.id != Ids::InvalidId32)
    {
        physx::PxRigidActor* actor = GetPxActor(id);
        if (actor != nullptr)
        {
            GET_ACTOR(id).feedback = (uint32_t)feedback;

            auto count = actor->getNbShapes();

            Util::StackArray<physx::PxShape*, DefaultShapeAlloc> buffer;
            PxShape** shapeBuffer = buffer.EmplaceArray(count);

            actor->getShapes(shapeBuffer, count);
            for (auto shape : buffer)
            {
                PxFilterData filter = shape->getSimulationFilterData();
                filter.word1 = feedback;
                shape->setSimulationFilterData(filter);
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
ActorId
ActorContext::CreateActor(const BodyDescription& desc)
{
    Scene& scene = Physics::GetScene(desc.scene);

    PxRigidActor* newActor = state.CreateActor(desc.type, desc.transform.position, desc.transform.rotation);
    ActorId id = ActorContext::AllocateActorId(newActor);
    scene.scene->addActor(*newActor);

    UpdateBody(id, desc);
    return id;
}

//------------------------------------------------------------------------------
/**
*/
void
ActorContext::UpdateBody(ActorId id, const BodyDescription& desc)
{
    n_assert(ActorContext::actorPool.IsValid(id.id));

    Actor& actor = GET_ACTOR(id);

    // the filter data is stored on every shape and the mass is distributed over the shapes, so both
    // only have to be touched when the body values they come from changed
    bool const massChanged = actor.mass != desc.mass;
    bool const filterChanged = actor.collisionGroup != desc.collisionGroup || actor.feedback != (uint32_t)desc.feedback;

    actor.mass = desc.mass;
    actor.collisionGroup = desc.collisionGroup;
    actor.feedback = desc.feedback;
    actor.userData = desc.userData;

    PxRigidActor* pxActor = ActorContext::GetPxActor(id);
    pxActor->setGlobalPose(Neb2PxTrans(desc.transform.position, desc.transform.rotation));

    if (pxActor->is<PxRigidDynamic>())
    {
        PxRigidDynamic* body = static_cast<PxRigidDynamic*>(pxActor);
        body->setLinearDamping(desc.linearDamping);
        body->setAngularDamping(desc.angularDamping);
        body->setRigidBodyFlag(PxRigidBodyFlag::eENABLE_CCD, desc.ccd);
        pxActor->setActorFlag(PxActorFlag::eDISABLE_GRAVITY, !desc.useGravity);
    }

    // Filter data is stored per shape so the shapes of the actor needs to be updated as well
    if (filterChanged)
    {
        for (ShapeId sid : actor.shapes)
        {
            if (GET_SHAPE(sid).shape != nullptr)
            {
                ApplyShapeFilterData(GET_SHAPE(sid).shape, id);
            }
        }
    }

    if (massChanged)
    {
        ApplyMass(id);
    }
}

//------------------------------------------------------------------------------
/**
*/
ShapeId
ActorContext::AddShape(ActorId actor, const ColliderDescription& desc)
{
    n_assert(ActorContext::actorPool.IsValid(actor.id));

    PxRigidActor* pxActor = ActorContext::GetPxActor(actor);
    PxGeometryHolder geometry;
    if (!BuildGeometry(desc, geometry))
        return ShapeId();

    PxMaterial* material = ResolveShapeMaterial(desc.materialId);
    PxShape* pxShape = PxRigidActorExt::createExclusiveShape(*pxActor, geometry.any(), *material);

    ShapeId id = ActorContext::AllocateShapeId(pxShape, actor, ColliderGeometryHash(desc), desc.userData);
    ApplyShapeState(GET_SHAPE(id), desc);
    ApplyMass(actor);
    return id;
}

//------------------------------------------------------------------------------
/**
    Apply a description onto an existing shape. Physx shapes can not change their geometry, so when the
    geometry hash changes the shape is replaced by a newly cooked one. Returns false if the geometry
    could not be built.
*/
bool
ActorContext::UpdateShape(ShapeId id, const ColliderDescription& desc)
{
    if (!ActorContext::IsValidShape(id))
        return false;

    Shape& shape = GET_SHAPE(id);
    PxShape* pxShape = shape.shape;
    if (pxShape == nullptr)
        return false;

    uint64_t hash = ColliderGeometryHash(desc);
    PxRigidActor* pxActor = pxShape->getActor();
    n_assert(pxActor != nullptr);

    bool const geometryChanged = hash != shape.geometryHash;
    bool const materialChanged = shape.materialId != desc.materialId;
    bool const triggerChanged = shape.isTrigger != desc.isTrigger;

    if (geometryChanged)
    {
        PxGeometryHolder geometry;
        if (!BuildGeometry(desc, geometry))
            return false;

        PxMaterial* material = ResolveShapeMaterial(desc.materialId);
        PxShape* newShape = PxRigidActorExt::createExclusiveShape(*pxActor, geometry.any(), *material);
        newShape->userData = (void*)(uintptr_t)id.id;

        // detaching an exclusively owned shape releases it
        pxActor->detachShape(*pxShape);
        shape.shape = newShape;
        shape.geometryHash = hash;
    }

    shape.materialId = desc.materialId;
    shape.isTrigger = desc.isTrigger;

    // the pose is the one thing which is cheap to apply and can change every frame
    shape.shape->setLocalPose(Neb2PxTrans(desc.transform));

    // a rebuilt shape already carries the material, but it always starts out as a simulation shape
    // with the default filter data
    if (materialChanged && !geometryChanged)
    {
        PxMaterial* material = ResolveShapeMaterial(desc.materialId);
        shape.shape->setMaterials(&material, 1);
    }
    if (triggerChanged || geometryChanged)
        ApplyShapeFlags(shape.shape, desc.isTrigger);
    if (geometryChanged)
        ApplyShapeFilterData(shape.shape, shape.actor);

    // the mass comes from the shape volumes and the material densities, so only those two changes
    // need the mass and inertia to be recomputed
    if (geometryChanged || materialChanged)
        ApplyMass(shape.actor);
    return true;
}

//------------------------------------------------------------------------------
/**
    Detach and release a shape from its actor
*/
void
ActorContext::RemoveShape(ShapeId id)
{
    if (!ActorContext::IsValidShape(id))
        return;

    Shape& shape = GET_SHAPE(id);
    PxShape* pxShape = shape.shape;
    if (pxShape == nullptr)
        return;

    PxRigidActor* pxActor = pxShape->getActor();
    if (pxActor != nullptr)
    {
        Actor& owner = GET_ACTOR(shape.actor);
        IndexT idx = owner.shapes.FindIndex(id);
        if (idx != InvalidIndex)
            owner.shapes.EraseSwap(owner.shapes.Begin() + idx);

        // detaching an exclusively owned shape releases it
        pxActor->detachShape(*pxShape);
    }

    shape.shape = nullptr;
    ActorContext::shapePool.Deallocate(id.id);

    if (pxActor != nullptr)
        ApplyMass(shape.actor);
}

//------------------------------------------------------------------------------
/**
*/
Shape&
ActorContext::GetShape(ShapeId id)
{
    return ActorContext::shapes[Ids::Index(id.id)];
}

//------------------------------------------------------------------------------
/**
*/
AggregateId
AggregateContext::AllocateAggregateId(AggregateResourceId res)
{
    AggregateId id;
    bool const allocated = AggregateContext::aggPool.Allocate(id.id);
    Ids::Id24 idx = Ids::Index(id.id);
    if (allocated)
    {
        AggregateContext::aggregates.Append(Physics::Aggregate());
        n_assert(idx == AggregateContext::aggregates.Size() - 1);
    }

    Aggregate& aggregate = AggregateContext::aggregates[idx];
    aggregate.id = id;
    aggregate.res = res;
    return id;
}

//------------------------------------------------------------------------------
/**
*/
Aggregate&
AggregateContext::GetAggregate(AggregateId id)
{
    n_assert(AggregateContext::aggPool.IsValid(id.id));
    return AggregateContext::aggregates[Ids::Index(id.id)];
}


//------------------------------------------------------------------------------
/**
*/
void
AggregateContext::DiscardAggregate(AggregateId id)
{
    n_assert(AggregateContext::aggPool.IsValid(id.id));
    Aggregate& aggregate = AggregateContext::GetAggregate(id);
    for (ActorId actorId : aggregate.actors)
    {
        Actor& actor = ActorContext::GetActor(actorId);
        auto scene = actor.actor->getScene();
        if (scene)
        {
            scene->removeActor(*actor.actor);
        }
        state.DiscardActor(actorId);
        actor.actor->release();
    }
        
    AggregateContext::aggPool.Deallocate(id.id);
}





}
