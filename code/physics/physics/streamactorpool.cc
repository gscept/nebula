//------------------------------------------------------------------------------
//  streamactorpool.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "PxConfig.h"
#include "physics/streamactorpool.h"
#include "physics/physxstate.h"
#include "physics/actorcontext.h"
#include "physics/utils.h"
#include "physics/geometry.h"
#include "physics/cooking.h"
#include "resources/resourceserver.h"
#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/physics/material.h"
#include "flat/physics/actor.h"

__ImplementClass(Physics::StreamActorPool, 'PSAP', Resources::ResourceLoader);

using namespace physx;

namespace Physics
{


static PxShape* GetShapeCopy(PxShape* shape, Math::vec3 const& scale);

StreamActorPool *actorPool = nullptr;
//------------------------------------------------------------------------------
/**
*/
StreamActorPool::StreamActorPool()
{
    this->streamerThreadName = "Physics Actor Pool Streamer Thread";
}

//------------------------------------------------------------------------------
/**
*/
StreamActorPool::~StreamActorPool()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
StreamActorPool::Setup()
{
    ResourceLoader::Setup();
    this->placeholderResourceName = IO::Path::File("system", "box", "actor");
    this->failResourceName = IO::Path::File("system", "box", "actor");
}

//------------------------------------------------------------------------------
/**
*/
ActorId
StreamActorPool::CreateActorInstance(PhysicsResourceId id, Math::transform const& trans, Physics::ActorType type, uint64_t userData, IndexT scene)
{
    n_assert2(this->resourceAllocator.Get<0>(id.id) == PhysicsResource::PhysicsResourceUnion_BodySetup, "Trying to create actor instance from non-actor resource");
    ActorResourceId actId = this->resourceAllocator.Get<1>(id.id);
    return this->CreateActorInstance(actId, trans, type, userData, scene);
}
//------------------------------------------------------------------------------
/**
*/
ActorId
StreamActorPool::CreateActorInstance(ActorResourceId id, Math::transform const& worldTrans, Physics::ActorType type, uint64_t userData, IndexT scene)
{
    n_assert(id.resourceId != Physics::InvalidPhysicsResourceId.resourceId);
    __LockName(this->actorAllocator, lock, id.id);
    ActorInfo& info = this->actorAllocator.Get<Info>(id.id);

    Math::transform trans = worldTrans * info.transform;
    bool isScaled = !Math::nearequal(trans.scale, Math::_plus1, 0.001f);
    

    info.instanceCount++;
    
    const auto& bi = info.body;
    physx::PxRigidActor* newActor = state.CreateActor(type, trans.position, trans.rotation);
        
    for (IndexT i = 0; i < info.body.shapes.Size(); i++)
    {
        PxShape* newShape = GetShapeCopy(info.body.shapes[i], trans.scale);
        newActor->attachShape(*newShape);
    }
    if (type == ActorType::Dynamic)
    {
        physx::PxRigidBodyExt::updateMassAndInertia(*static_cast<physx::PxRigidDynamic*>(newActor), info.body.densities.Begin(), info.body.densities.Size());
    }


    GetScene(scene).scene->addActor(*newActor);

    ActorId newId = ActorContext::AllocateActorId(newActor, id);
    Actor& actor = ActorContext::GetActor(newId);
    actor.userData = userData;

#if NEBULA_DEBUG
    actor.debugName = Util::String::Sprintf("%s %d", this->names[id.resourceId].AsString().AsCharPtr(), newId);
    newActor->setName(actor.debugName.AsCharPtr());
#endif

    return newId;
}

//------------------------------------------------------------------------------
/**
*/
void 
StreamActorPool::DiscardActorInstance(ActorId id)
{
    if(!ActorContext::IsValid(id))
    {
        return;
    }
    Actor& actor = ActorContext::GetActor(id);
    if (actor.res != ActorResourceId::Invalid())
    {
        __LockName(this->actorAllocator, lock, actor.res.id);
        ActorInfo& info = this->actorAllocator.Get<Info>(actor.res.id);
        info.instanceCount--;
    }
    ActorContext::DiscardActor(id);
}

//------------------------------------------------------------------------------
/**
*/
AggregateId 
StreamActorPool::CreateAggregate(PhysicsResourceId id, Math::transform const& trans, Physics::ActorType type, uint64_t userData, IndexT scene)
{
    n_assert2(this->resourceAllocator.Get<0>(id.id) == PhysicsResource::PhysicsResourceUnion_Aggregate, "Trying to create aggregate actor instance from invalid resource");
    __LockName(this->aggregateAllocator, lock, id.id);
    AggregateResourceId aggId = this->resourceAllocator.Get<1>(id.id);
    AggregateInfo& info = this->aggregateAllocator.Get<Info>(aggId.id);
    info.instanceCount++;

    AggregateId aggInstanceId = AggregateContext::AllocateAggregateId(aggId);
    Aggregate aggregate = AggregateContext::GetAggregate(aggInstanceId);
    Util::Dictionary<Util::StringAtom, physx::PxActor*> actorDict;
    for (ActorResourceId body : info.bodies)
    {
        ActorId actorId = this->CreateActorInstance(body, trans, type, userData, scene);
        Actor& actor = ActorContext::GetActor(actorId);
        const ActorInfo & actorInfo = this->actorAllocator.Get<0>(actor.res.id);
        actorDict.Add(actorInfo.name, actor.actor);
        aggregate.actors.Append(actorId);
    }
    return aggInstanceId;
}

//------------------------------------------------------------------------------
/**
*/
PhysicsResource::PhysicsResourceUnion 
StreamActorPool::GetResourceType(PhysicsResourceId id)
{
    return this->resourceAllocator.Get<0>(id.id);
}


//------------------------------------------------------------------------------
/**
*/
static PxShape* CreateColliderShape(physx::PxGeometryHolder geometry, physx::PxMaterial* material, physx::PxTransform const& trans, const Util::String& colliderName, bool exclusive)
{
    physx::PxShape* newShape = state.physics->createShape(geometry.any(), *material, exclusive);
    n_assert(newShape != nullptr);
#ifdef NEBULA_DEBUG
    newShape->setName(colliderName.AsCharPtr());
#endif
    newShape->setLocalPose(trans);
    return newShape;
};

//------------------------------------------------------------------------------
/**
*/
static void AddCollider(physx::PxGeometryHolder geometry, IndexT material, const Math::transform& trans, const char* name, const Util::String& colliderName, BodyInfo& bodyInfo, const Util::StringAtom& tag, Ids::Id32 entry)
{
    const Physics::Material& mat = GetMaterial(material);
    bodyInfo.colliders.Append(colliderName);
    Util::String shapeDebugName;
#ifdef NEBULA_DEBUG
    shapeDebugName = Util::String::Sprintf("%s %s %s %d", name, colliderName.AsCharPtr(), tag.Value(), entry);
    bodyInfo.shapeDebugNames.Append(shapeDebugName);
#endif
    PxShape* newShape = CreateColliderShape(geometry, mat.material, Neb2PxTrans(trans), shapeDebugName, true);
    bodyInfo.shapes.Append(newShape);
    bodyInfo.densities.Append(GetMaterial(material).density);
}

//------------------------------------------------------------------------------
/**
*/
static void
AddMeshColliders(PhysicsResource::MeshColliderT* colliderNode, Math::transform const& nodeTransform, Util::String nodeName, IndexT materialIdx, const Util::StringAtom& tag, Ids::Id32 entry, BodyInfo& targetBody)
{
    const GeometryLoader& loader = GetGeometryLoader();
    if (!loader.LoadMesh)
    {
        n_error("AddMeshColliders: no geometry loader registered, cannot read '%s'", colliderNode->file.AsCharPtr());
        return;
    }

    Util::StringAtom resource = colliderNode->file;

    MeshGeometryInput input;
    input.topology = colliderNode->type;
    if (!loader.LoadMesh(resource, colliderNode->prim_group, input))
    {
        return;
    }

    if (colliderNode->type == MeshTopology_ApproxSkin)
    {
        Util::Array<CookedConvex> cooked = CookApproxSkin(input);
        for (SizeT i = 0; i < cooked.Size(); i++)
        {
            Util::String boneName = Util::String::Sprintf("%s_%d", nodeName.AsCharPtr(), cooked[i].bone);
            AddCollider(cooked[i].geometry, materialIdx, nodeTransform, resource.Value(), boneName, targetBody, tag, entry);
        }
        return;
    }

    physx::PxGeometryHolder geometry = CookMesh(input);
    AddCollider(geometry, materialIdx, nodeTransform, resource.Value(), nodeName, targetBody, tag, entry);
}

//------------------------------------------------------------------------------
/**
*/
static void
AddHeightField(PhysicsResource::HeightFieldColliderT* colliderNode, Math::transform const& nodeTransform, Util::String nodeName, IndexT materialIdx, const Util::StringAtom& tag, Ids::Id32 entry, BodyInfo& targetBody)
{
    const GeometryLoader& loader = GetGeometryLoader();
    if (!loader.LoadHeightField)
    {
        n_error("AddHeightField: no geometry loader registered, cannot read '%s'", colliderNode->file.AsCharPtr());
        return;
    }

    HeightFieldInput input;
    if (!loader.LoadHeightField(colliderNode->file, input))
    {
        return;
    }

    physx::PxGeometryHolder geometry =
        CookHeightField(input, colliderNode->height_range, colliderNode->target_size_x, colliderNode->target_size_y);

    // the heightfield origin is at the corner of the field, move it to the center
    Math::transform offsetTransform;
    offsetTransform.position = Math::vec3(-0.5f * colliderNode->target_size_x, 0.0f, -0.5f * colliderNode->target_size_y);
    AddCollider(geometry, materialIdx, offsetTransform, colliderNode->file.AsCharPtr(), nodeName, targetBody, tag, entry);
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceLoader::ResourceInitOutput
StreamActorPool::InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream)
{
    n_assert(stream.isvalid());    
    Resources::ResourceLoader::ResourceInitOutput ret;
    
    PhysicsResource::ActorT actor;
    Flat::FlatbufferInterface::DeserializeFlatbuffer<PhysicsResource::Actor>(actor, (uint8_t*)stream->Map());

       
    static const auto parseBody = [](const PhysicsResource::BodyT& body, const ResourceLoadJob& job)
        {
            BodyInfo bodyInfo;
            bodyInfo.feedbackFlag = body.feedback;
            for (auto const& shape : body.shapes)
            {
                auto const& collider = shape->collider;
                Util::StringAtom matAtom = shape->material;
                IndexT material = LookupMaterial(matAtom);

                switch (collider->type)
                {
                case ColliderType_Sphere:
                {

                    float radius = collider->data.AsSphereCollider()->radius;
                    physx::PxGeometryHolder geometry = PxSphereGeometry(radius);
                    AddCollider(geometry, material, shape->transform, "Sphere", collider->name, bodyInfo, job.tag, job.id.loaderInstanceId);
                }
                break;
                case ColliderType_Box:
                {
                    Math::vector extents = collider->data.AsBoxCollider()->extents;
                    physx::PxGeometryHolder geometry = PxBoxGeometry(Neb2PxVec(extents));
                    AddCollider(geometry, material, shape->transform, "Box", collider->name, bodyInfo, job.tag, job.id.loaderInstanceId);
                }
                break;
                case ColliderType_Plane:
                {
                    // plane is defined via transform of the actor
                    physx::PxGeometryHolder geometry = PxPlaneGeometry();
                    AddCollider(geometry, material, shape->transform, "Plane", collider->name, bodyInfo, job.tag, job.id.loaderInstanceId);
                }
                break;
                case ColliderType_Capsule:
                {
                    auto capsule = collider->data.AsCapsuleCollider();
                    float radius = capsule->radius;
                    float halfHeight = capsule->halfheight;
                    physx::PxGeometryHolder geometry = PxCapsuleGeometry(radius, halfHeight);
                    AddCollider(geometry, material, shape->transform, "Capsule", collider->name, bodyInfo, job.tag, job.id.loaderInstanceId);
                }
                break;
                case ColliderType_Mesh:
                {
                    AddMeshColliders(collider->data.AsMeshCollider(), shape->transform, collider->name, material, job.tag, job.id.loaderInstanceId, bodyInfo);
                }
                break;
                case ColliderType_HeightField:
                {
                    AddHeightField(collider->data.AsHeightFieldCollider(), shape->transform, collider->name, material, job.tag, job.id.loaderInstanceId, bodyInfo);
                }
                break;
                default:
                    n_error("unknown collider type");
                }
            }
            return bodyInfo;
        };

    Ids::Id32 resId = this->resourceAllocator.Alloc();
    this->resourceAllocator.Set<0>(resId, actor.data.type);
    switch (actor.data.type)
    {
        case PhysicsResource::PhysicsResourceUnion_BodySetup:
        {
            /// during the load-phase, we can safetly get the structs
            ActorInfo actorInfo;
            actorInfo.instanceCount = 0;

            const PhysicsResource::BodySetupT* bodySetup = actor.data.AsBodySetup();
            auto& body = actorInfo.body;
            body = parseBody(*bodySetup->body, job);
            actorInfo.name = bodySetup->name;
            actorInfo.transform = bodySetup->transform;

            Ids::Id32 id = this->actorAllocator.Alloc();
            this->actorAllocator.Set<Info>(id, actorInfo);
            this->resourceAllocator.Set<1>(resId, id);
        }
        break;
        case PhysicsResource::PhysicsResourceUnion_Aggregate:
        {
            AggregateInfo aggInfo;
            aggInfo.instanceCount = 0;
            const PhysicsResource::AggregateT* aggSetup = actor.data.AsAggregate();
            aggInfo.bodies.Reserve((SizeT)aggSetup->bodies.size());
            for (auto const& bodySetup : aggSetup->bodies)
            {
                Ids::Id32 actorId = this->actorAllocator.Alloc();
                aggInfo.bodies.Append(actorId);

                ActorInfo actorInfo;
                actorInfo.instanceCount = 0;                
                actorInfo.body = parseBody(*bodySetup->body, job);
                actorInfo.name = bodySetup->name;
                actorInfo.transform = bodySetup->transform;
                actorAllocator.Set<Info>(actorId, actorInfo);
            }
            Ids::Id32 aggId = this->aggregateAllocator.Alloc();
            this->aggregateAllocator.Set<Info>(aggId, aggInfo);
            this->resourceAllocator.Set<1>(resId, aggId);
        }
        break;
        default:
            ret.id = Physics::InvalidPhysicsResourceId;
            return ret;
    }
    ret.id = resId;
    return ret;    
}

//------------------------------------------------------------------------------
/**
*/
void
StreamActorPool::Unload(const Resources::ResourceId id)
{
    __LockName(this->resourceAllocator, lock, id.resource);
    
    static const auto freeShapes = [&](Physics::ActorInfo& actor)
        {
            n_assert2(actor.instanceCount == 0, "Actor has active Instances");

            for (auto i : actor.body.shapes)
            {
                i->release();
            }
            actor.body.shapes.Clear();
        };
    Ids::Id32 resId = this->resourceAllocator.Get<1>(id.resource);
    switch (this->resourceAllocator.Get<0>(id.resource))
    {
        case PhysicsResource::PhysicsResourceUnion_BodySetup:
        {
            ActorInfo& info = this->actorAllocator.Get<0>(resId);
            freeShapes(info);
            actorAllocator.Dealloc(resId);
	    }
        break;
        case PhysicsResource::PhysicsResourceUnion_Aggregate:
        {
            AggregateInfo& info = this->aggregateAllocator.Get<0>(resId);
            for (ActorResourceId actorId : info.bodies)
            {
                ActorInfo& actor = this->actorAllocator.Get<0>(actorId.id);
                freeShapes(actor);
                this->actorAllocator.Dealloc(actorId.id);
            }
            this->aggregateAllocator.Dealloc(resId);
        }
        break;
    }    
    this->resourceAllocator.Dealloc(id.resource);
}


static PxShape* GetShapeCopy(PxShape* shape, Math::vec3 const& inScale)
{
    n_assert(shape != nullptr);
    physx::PxTransform localTrans = shape->getLocalPose();
    const physx::PxGeometry& geometry = shape->getGeometry();
    physx::PxGeometryType::Enum type = geometry.getType();
    physx::PxVec3 scale = Neb2PxVec(inScale);

    //localTrans.p = localTrans.p.multiply(scale);
    PxGeometryHolder holder;
    Util::String debugName;
#ifdef NEBULA_DEBUG
    
#endif
    PxShape* newShape = nullptr;
    switch (type)
    {
        case physx::PxGeometryType::eSPHERE:
        {
            const float radiusScale = scale.maxElement();
            PxSphereGeometry sGeom = static_cast<const PxSphereGeometry&>(geometry);
            const float newRadius = sGeom.radius * radiusScale;
            holder = PxSphereGeometry(newRadius);
        }
        break;
        case physx::PxGeometryType::eCAPSULE:
        {   
            PxCapsuleGeometry cGeom = static_cast<const PxCapsuleGeometry&>(geometry);
            holder = PxCapsuleGeometry(cGeom.radius * scale.x, cGeom.halfHeight * scale.z);
        }
        break;
        case physx::PxGeometryType::eBOX:
        {
            PxBoxGeometry bGeom = static_cast<const PxBoxGeometry&>(geometry);
            physx::PxVec3 half = bGeom.halfExtents;
            half = half.multiply(scale);
            holder = PxBoxGeometry(half);
        }
        break;
        case physx::PxGeometryType::eCONVEXMESH:
        {
            physx::PxConvexMeshGeometry geom = static_cast<const PxConvexMeshGeometry&>(geometry);
            physx::PxMeshScale meshScale(scale, physx::PxQuat(physx::PxIDENTITY::PxIdentity));
            holder = physx::PxConvexMeshGeometry(geom.convexMesh, meshScale);
        }
        break;
        case physx::PxGeometryType::eTRIANGLEMESH:
        {
            physx::PxTriangleMeshGeometry geom = static_cast<const PxTriangleMeshGeometry&>(geometry);
            physx::PxMeshScale meshScale(scale, physx::PxQuat(physx::PxIDENTITY::PxIdentity));
            holder = physx::PxTriangleMeshGeometry(geom.triangleMesh, meshScale);
        }
        break;
        case physx::PxGeometryType::eHEIGHTFIELD:
        {
            // we never copy these
            return shape;
        }
        break;
        default:
            n_assert_fmt(false, "unsupported mesh type %d", type);
    }
    const uint16_t mats = shape->getNbMaterials();
    physx::PxMaterial* mat[16];
    n_assert(mats < 16);
    shape->getMaterials((physx::PxMaterial **) &mat, 16, 0);
    return CreateColliderShape(holder, mat[0], localTrans, debugName, true);
}
}
