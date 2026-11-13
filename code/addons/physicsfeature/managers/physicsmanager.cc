//------------------------------------------------------------------------------
//  physicsmanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "physicsmanager.h"
#include "game/gameserver.h"
#include "physics/charactercontext.h"
#include "physicsinterface.h"
#include "physics/actorcontext.h"
#include "resources/resourceserver.h"
#include "components/physicsfeature.h"
#include "basegamefeature/components/basegamefeature.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"
#include "basegamefeature/components/velocity.h"
#include "basegamefeature/managers/transformhierarchy.h"
#include "physicsfeature/physicsfeatureunit.h"

namespace PhysicsFeature
{

__ImplementClass(PhysicsFeature::PhysicsManager, 'PhMa', Game::Manager);
__ImplementSingleton(PhysicsManager)

//------------------------------------------------------------------------------
/**
*/
PhysicsManager::PhysicsManager()
{
    __ConstructSingleton
}

//------------------------------------------------------------------------------
/**
*/
PhysicsManager::~PhysicsManager()
{
    __DestructSingleton
}

//------------------------------------------------------------------------------
/**
    Resolves which entity owns the physx actor a collider belongs to. A collider on a body entity
    resolves to itself, a collider on a child entity resolves to the body of its parent.
*/
static Game::Entity
ResolveBodyEntity(Game::World* world, Game::Entity entity)
{
    for (int i = 0; i < 16 && entity != Game::Entity::Invalid(); i++)
    {
        // an entity which is being deallocated is walked through by its component decay callbacks, and the
        // parent it names can already be gone with it
        if (!world->IsValid(entity))
            break;

        if (world->HasComponent<PhysicsFeature::Rigidbody>(entity) || world->HasComponent<PhysicsFeature::StaticBody>(entity))
            return entity;

        if (!world->HasComponent<Game::HTransform>(entity))
            break;

        Game::Entity const parent = world->GetComponent<Game::HTransform>(entity).parent;
        if (parent == entity)
            break;
        entity = parent;
    }

    return Game::Entity::Invalid();
}

//------------------------------------------------------------------------------
/**
    Checks if the actorid is valid, and if the actor points to the correct entity,
    and returns a valid or invalid actor id based on the check.
*/
static Physics::ActorId
GetValidateActorId(Game::Entity entity, PhysicsFeature::PhysicsActor const& actor)
{
    Physics::ActorId const handle(actor.actorId);
    if (handle.id == Ids::InvalidId32 || !Physics::ActorContext::IsValid(handle))
        return Physics::ActorId(Ids::InvalidId32);
    
    if (Physics::ActorContext::GetActor(handle).userData != (uint64_t)Ids::Id64(entity))
        return Physics::ActorId(Ids::InvalidId32);
        
    return handle;
}

//------------------------------------------------------------------------------
/**
    get the physx actor of the entity
*/
static Physics::ActorId
ResolveActorHandle(Game::World* world, Game::Entity entity)
{
    if (!world->HasComponent<PhysicsFeature::PhysicsActor>(entity))
        return Physics::ActorId(Ids::InvalidId32);

    PhysicsFeature::PhysicsActor actorId = world->GetComponent<PhysicsFeature::PhysicsActor>(entity);
    return GetValidateActorId(entity, actorId);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::Mark(Game::World* world, Game::Entity entity, uint32_t pending)
{
    if (entity == Game::Entity::Invalid() || !world->IsValid(entity))
        return;

    if (!world->HasComponent<PhysicsFeature::PhysicsActor>(entity))
    {
        // the entity has no handle yet, which means its body or collider components were added this
        // frame and are not in the tables yet. Their inits ensure a handle, and a fresh handle
        // queues everything, so this mark is covered by that build
        return;
    }

    n_assert(PhysicsManager::HasInstance());

    PhysicsFeature::PhysicsActor self = world->GetComponent<PhysicsFeature::PhysicsActor>(entity);
    if (self.pending == 0)
        PhysicsManager::Instance()->rebuildQueue.Append(entity);

    self.pending |= pending;
    world->SetComponent(entity, self);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::MarkShapeChanged(Game::World* world, Game::Entity entity)
{
    if (entity == Game::Entity::Invalid() || !world->HasComponent<PhysicsFeature::PhysicsActor>(entity))
        return;

    PhysicsManager::Mark(world, entity, PendingShape);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::MarkBodyChanged(Game::World* world, Game::Entity entity)
{
    PhysicsManager::Mark(world, entity, PendingBody | PendingShape);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::MarkPoseChanged(Game::World* world, Game::Entity entity)
{
    PhysicsManager::Mark(world, entity, PendingPose);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::MarkComponentChange(Game::World* world, Game::Entity entity, Game::ComponentId component)
{
    if (component == Game::GetComponentId<PhysicsFeature::PhysicsResource>() ||
        component == Game::GetComponentId<PhysicsFeature::Rigidbody>() ||
        component == Game::GetComponentId<PhysicsFeature::StaticBody>())
    {
        PhysicsManager::MarkBodyChanged(world, entity);
        return;
    }

    if (component == Game::GetComponentId<PhysicsFeature::BoxCollider>() ||
        component == Game::GetComponentId<PhysicsFeature::SphereCollider>() ||
        component == Game::GetComponentId<PhysicsFeature::CapsuleCollider>() ||
        component == Game::GetComponentId<PhysicsFeature::PlaneCollider>() ||
        component == Game::GetComponentId<PhysicsFeature::MeshCollider>() ||
        component == Game::GetComponentId<PhysicsFeature::HeightFieldCollider>())
    {
        PhysicsManager::MarkShapeChanged(world, entity);
        return;
    }

    if (component == Game::GetComponentId<Game::Position>() ||
        component == Game::GetComponentId<Game::Orientation>() ||
        component == Game::GetComponentId<Game::Scale>() ||
        component == Game::GetComponentId<Game::HTransform>())
    {
        PhysicsManager::MarkPoseChanged(world, entity);
    }
}

//------------------------------------------------------------------------------
/**
    True if the actor is a rigid body, which covers both the kinematic and the dynamic actor type.
*/
static bool
ActorIsDynamic(Physics::ActorId id)
{
    physx::PxActor* pxActor = Physics::ActorContext::GetActor(id).actor;
    return pxActor != nullptr && pxActor->is<physx::PxRigidBody>();
}

//------------------------------------------------------------------------------
/**
    Converts from pxshape to our internal collider type enum
*/
static Physics::ColliderType
ShapeColliderType(physx::PxShape const* pxShape)
{
    if (pxShape == nullptr)
        return (Physics::ColliderType)-1;

    switch (pxShape->getGeometry().getType())
    {
    case physx::PxGeometryType::eBOX:
        return Physics::ColliderType_Box;
    case physx::PxGeometryType::eSPHERE:
        return Physics::ColliderType_Sphere;
    case physx::PxGeometryType::eCAPSULE:
        return Physics::ColliderType_Capsule;
    case physx::PxGeometryType::ePLANE:
        return Physics::ColliderType_Plane;
    case physx::PxGeometryType::eHEIGHTFIELD:
        return Physics::ColliderType_HeightField;
    case physx::PxGeometryType::eCONVEXMESH:
    case physx::PxGeometryType::eTRIANGLEMESH:
        return Physics::ColliderType_Mesh;
    default:
        return (Physics::ColliderType)-1;
    }
}

//------------------------------------------------------------------------------
/**
    True if the entity still has the collider component matching a physx shape of this type.
*/
static bool
HasColliderOfType(Game::World* world, Game::Entity entity, int colliderClass)
{
    switch (colliderClass)
    {
    case Physics::ColliderType_Box:
        return world->HasComponent<PhysicsFeature::BoxCollider>(entity);
    case Physics::ColliderType_Sphere:
        return world->HasComponent<PhysicsFeature::SphereCollider>(entity);
    case Physics::ColliderType_Capsule:
        return world->HasComponent<PhysicsFeature::CapsuleCollider>(entity);
    case Physics::ColliderType_Plane:
        return world->HasComponent<PhysicsFeature::PlaneCollider>(entity);
    case Physics::ColliderType_Mesh:
        return world->HasComponent<PhysicsFeature::MeshCollider>(entity);
    case Physics::ColliderType_HeightField:
        return world->HasComponent<PhysicsFeature::HeightFieldCollider>(entity);
    }

    return false;
}

//------------------------------------------------------------------------------
/**
    True when a shape name held by a handle still names the shape its entity built. 
*/
static bool
ShapeBelongsTo(Physics::ShapeId id, Game::Entity entity)
{
    if (!Physics::ActorContext::IsValidShape(id))
        return false;

    return Physics::ActorContext::GetShape(id).userData == (uint64_t)Ids::Id64(entity);
}

//------------------------------------------------------------------------------
/**
*/
static Physics::ShapeId
FindShapeForEntity(Physics::ActorId actorId, Game::Entity entity)
{
    Physics::Actor const& actor = Physics::ActorContext::GetActor(actorId);
    uint64_t const userData = (uint64_t)Ids::Id64(entity);

    for (Physics::ShapeId const shapeId : actor.shapes)
    {
        if (!Physics::ActorContext::IsValidShape(shapeId))
            continue;

        if (Physics::ActorContext::GetShape(shapeId).userData == userData)
            return shapeId;
    }

    return Physics::ShapeId(Ids::InvalidId32);
}

//------------------------------------------------------------------------------
/**
    True when the entity has a collider component other than this one. An entity is only allowed to have one shape.
*/
static bool
HasOtherCollider(Game::World* world, Game::Entity entity, Game::ComponentId component)
{
    static const Game::ComponentId colliders[] = {
        Game::GetComponentId<PhysicsFeature::BoxCollider>(),
        Game::GetComponentId<PhysicsFeature::SphereCollider>(),
        Game::GetComponentId<PhysicsFeature::CapsuleCollider>(),
        Game::GetComponentId<PhysicsFeature::PlaneCollider>(),
        Game::GetComponentId<PhysicsFeature::MeshCollider>(),
        Game::GetComponentId<PhysicsFeature::HeightFieldCollider>()
    };

    for (Game::ComponentId const collider : colliders)
    {
        if (collider != component && world->HasComponent(entity, collider))
            return true;
    }

    return false;
}

//------------------------------------------------------------------------------
/**
*/
Physics::ShapeId
PhysicsManager::GetShapeForEntity(Game::World* world, Game::Entity entity)
{
    if (entity == Game::Entity::Invalid() || !world->IsValid(entity))
        return Physics::ShapeId(Ids::InvalidId32);

    if (!world->HasComponent<PhysicsFeature::PhysicsActor>(entity))
        return Physics::ShapeId(Ids::InvalidId32);

    PhysicsFeature::PhysicsActor handle = world->GetComponent<PhysicsFeature::PhysicsActor>(entity);
    Physics::ShapeId const shape(handle.shapeId);
    if (ShapeBelongsTo(shape, entity))
        return shape;

    Physics::ActorId actor = ResolveActorHandle(world, entity);
    if (actor.id == Ids::InvalidId32)
    {
        Game::Entity const body = ResolveBodyEntity(world, entity);
        if (body != Game::Entity::Invalid() && world->IsValid(body))
            actor = ResolveActorHandle(world, body);
    }

    if (actor.id == Ids::InvalidId32)
        return Physics::ShapeId(Ids::InvalidId32);

    Physics::ShapeId const found = FindShapeForEntity(actor, entity);
    if (!Physics::ActorContext::IsValidShape(found))
        return found;

    handle.shapeId = found.id;
    world->SetComponent(entity, handle);
    return found;
}

//------------------------------------------------------------------------------
/**
*/
static void
ClearShapeField(Game::World* world, Game::Entity entity)
{
    if (entity == Game::Entity::Invalid() || !world->IsValid(entity))
        return;

    if (!world->HasComponent<PhysicsFeature::PhysicsActor>(entity))
        return;

    PhysicsFeature::PhysicsActor handle = world->GetComponent<PhysicsFeature::PhysicsActor>(entity);
    handle.shapeId = Ids::InvalidId32;
    world->SetComponent(entity, handle);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::ReleaseShapeForEntity(Game::World* world, Game::Entity entity)
{
    if (entity == Game::Entity::Invalid() || !world->IsValid(entity))
        return;

    if (!world->HasComponent<PhysicsFeature::PhysicsActor>(entity))
    {
        // the handle is gone with the entity, which is the case when the entity is deleted. Its decay queues the
        // walk for the body the shape was attached to, so the shape is not lost
        PhysicsManager::Mark(world, entity, PendingShape | PendingStructural);
        return;
    }

    PhysicsFeature::PhysicsActor handle = world->GetComponent<PhysicsFeature::PhysicsActor>(entity);
    Physics::ShapeId const shape(handle.shapeId);
    if (ShapeBelongsTo(shape, entity))
    {
        Physics::ActorContext::RemoveShape(shape);
        handle.shapeId = Ids::InvalidId32;
        world->SetComponent(entity, handle);
        return;
    }

    PhysicsManager::Mark(world, entity, PendingShape | PendingStructural);
}

//------------------------------------------------------------------------------
/**
    Releases the shapes of an actor whose collider component is gone, or whose collider entity has
    been moved to another body. Shapes that came along with an actor resource are not tracked by the
    actor and are left alone. This is the fallback for an entity whose handle cannot name the shape it
    built; an entity whose handle can name it releases that shape by itself, on the decay of its component.
*/
static void
GarbageCollectShapes(Game::World* world, Game::Entity bodyEntity, Physics::ActorId actorId)
{
    Physics::Actor& actor = Physics::ActorContext::GetActor(actorId);

    // releasing a shape swaps the last shape the actor tracks into the freed slot, so the index is
    // stepped back to make sure that one is looked at too
    for (IndexT i = 0; i < actor.shapes.Size(); i++)
    {
        Physics::ShapeId const shapeId = actor.shapes[i];
        if (!Physics::ActorContext::IsValidShape(shapeId))
            continue;

        Physics::Shape const& shape = Physics::ActorContext::GetShape(shapeId);
        int const colliderType = ShapeColliderType(shape.shape);
        if (colliderType < 0)
            continue;

        Game::Entity const owner = Game::Entity::FromId((Ids::Id32)shape.userData);
        if (owner == Game::Entity::Invalid() || !world->IsValid(owner)
            || !HasColliderOfType(world, owner, colliderType)
            || ResolveBodyEntity(world, owner) != bodyEntity)
        {
            Physics::ActorContext::RemoveShape(shapeId);
            ClearShapeField(world, owner);
            --i;
        }
    }
}

//------------------------------------------------------------------------------
/**
    Queues the entities whose shapes hang off an actor which is about to be released, so that their
    shapes get built again on whichever actor they end up on. Only shapes built from a collider
    description are tracked by the actor, and those are the only ones owned by an entity.
*/
static void
MarkOwnedShapes(Game::World* world, Physics::ActorId actorId)
{
    Physics::Actor const& actor = Physics::ActorContext::GetActor(actorId);
    for (Physics::ShapeId const shapeId : actor.shapes)
    {
        if (!Physics::ActorContext::IsValidShape(shapeId))
            continue;

        Game::Entity const owner = Game::Entity::FromId((Ids::Id32)Physics::ActorContext::GetShape(shapeId).userData);
        PhysicsManager::Mark(world, owner, PendingShape);
    }
}

//------------------------------------------------------------------------------
/**
*/
static void
BuildActorFromPhysicsResource(Game::World* world, Game::Entity entity, PhysicsFeature::PhysicsActor& self)
{
    Physics::ActorId const current = GetValidateActorId(entity, self);
    if (current.id != Ids::InvalidId32 && Physics::ActorContext::GetActor(current).res != Physics::ActorResourceId::Invalid())
        return;

    if (current.id != Ids::InvalidId32)
    {
        // the entity went from a described body to a resource driven one, which is a different kind
        MarkOwnedShapes(world, current);
        Physics::DestroyActorInstance(current);
    }

    PhysicsFeature::PhysicsResource const actor = world->GetComponent<PhysicsFeature::PhysicsResource>(entity);

    Math::transform const worldTransform(
        world->GetComponent<Game::Position>(entity),
        world->GetComponent<Game::Orientation>(entity),
        world->GetComponent<Game::Scale>(entity)
    );

    Resources::ResourceId resId = Resources::CreateResource(actor.resource, "PHYS", nullptr, nullptr, true);
    Physics::ActorId const actorid = Physics::CreateActorInstance(
        resId, worldTransform, (Physics::ActorType)actor.actorType, Ids::Id64(entity), PhysicsFeatureUnit::Instance()->SceneForWorld(world));
    self.actorId = actorid.id;
    self.body = entity;

    if ((int)actor.actorType == Physics::Kinematic)
    {
        world->AddComponent<PhysicsFeature::IsKinematic>(entity);
    }

    if (Physics::ActorContext::IsValid(actorid))
    {
        if (world->HasComponent<Game::Velocity>(entity))
        {
            Physics::ActorContext::SetLinearVelocity(actorid, world->GetComponent<Game::Velocity>(entity));
        }
        if (world->HasComponent<Game::AngularVelocity>(entity))
        {
            Physics::ActorContext::SetAngularVelocity(actorid, world->GetComponent<Game::AngularVelocity>(entity));
        }
        // fixme, this should be configured, just brute force it for now
        Physics::ActorContext::SetCollisionFeedback(actorid, Physics::CollisionFeedback_Full);
    }
}

//------------------------------------------------------------------------------
/**
*/
static void
BuildRigidbody(Game::World* world, Game::Entity entity, PhysicsFeature::PhysicsActor& actor)
{
    PhysicsFeature::Rigidbody const body = world->GetComponent<PhysicsFeature::Rigidbody>(entity);

    Physics::BodyDescription desc;
    desc.type = (Physics::ActorType)body.actorType;
    desc.scene = PhysicsFeatureUnit::Instance()->SceneForWorld(world);
    desc.mass = body.mass;
    desc.linearDamping = body.linearDamping;
    desc.angularDamping = body.angularDamping;
    desc.useGravity = body.useGravity;
    desc.ccd = body.ccd;
    desc.collisionGroup = body.collisionGroup;
    desc.feedback = (Physics::CollisionFeedback)body.collisionFeedback;
    desc.userData = (uint64_t)Ids::Id64(entity);
    desc.transform = Math::transform(
        world->GetComponent<Game::Position>(entity),
        world->GetComponent<Game::Orientation>(entity),
        Math::_plus1
    );

    bool const wantDynamic = body.actorType != PhysicsFeature::ActorType::Static;
    Physics::ActorId current = GetValidateActorId(entity, actor);

    if (current.id != Ids::InvalidId32)
    {
        bool const isResourceActor = Physics::ActorContext::GetActor(current).res != Physics::ActorResourceId::Invalid();

        if (isResourceActor || ActorIsDynamic(current) != wantDynamic)
        {
            // PhysX cannot turn a static actor into a rigid body, and a resource driven actor is not
            // a described one, so the actor is released and built again below
            MarkOwnedShapes(world, current);
            Physics::DestroyActorInstance(current);
            current = Physics::ActorId(Ids::InvalidId32);
        }
    }

    if (current.id == Ids::InvalidId32)
    {
        actor.actorId = Physics::ActorContext::CreateActor(desc).id;

        if (world->HasComponent<Game::Velocity>(entity))
            Physics::ActorContext::SetLinearVelocity(Physics::ActorId(actor.actorId), world->GetComponent<Game::Velocity>(entity));
        if (world->HasComponent<Game::AngularVelocity>(entity))
            Physics::ActorContext::SetAngularVelocity(Physics::ActorId(actor.actorId), world->GetComponent<Game::AngularVelocity>(entity));
    }
    else
    {
        Physics::ActorContext::UpdateBody(current, desc);
    }

    actor.body = entity;
}

//------------------------------------------------------------------------------
/**
*/
static void
BuildStaticBody(Game::World* world, Game::Entity entity, PhysicsFeature::PhysicsActor& actor)
{
    PhysicsFeature::StaticBody const body = world->GetComponent<PhysicsFeature::StaticBody>(entity);

    Physics::BodyDescription desc;
    desc.type = Physics::Static;
    desc.scene = PhysicsFeatureUnit::Instance()->SceneForWorld(world);
    desc.collisionGroup = body.collisionGroup;
    desc.feedback = (Physics::CollisionFeedback)body.collisionFeedback;
    desc.userData = (uint64_t)Ids::Id64(entity);
    desc.transform = Math::transform(
        world->GetComponent<Game::Position>(entity),
        world->GetComponent<Game::Orientation>(entity),
        Math::_plus1
    );

    Physics::ActorId current = GetValidateActorId(entity, actor);

    if (current.id != Ids::InvalidId32)
    {
        Physics::Actor& actor = Physics::ActorContext::GetActor(current);

        if (actor.res != Physics::ActorResourceId::Invalid() || ActorIsDynamic(current))
        {
            MarkOwnedShapes(world, current);
            Physics::DestroyActorInstance(current);
            current = Physics::ActorId(Ids::InvalidId32);
        }
    }

    if (current.id == Ids::InvalidId32)
    {
        actor.actorId = Physics::ActorContext::CreateActor(desc).id;
    }
    else
    {
        Physics::ActorContext::UpdateBody(current, desc);
    }

    actor.body = entity;
}

//------------------------------------------------------------------------------
/**
*/
static bool
BuildCollider(Game::World* world, Game::Entity entity, PhysicsFeature::PhysicsActor& actor, Physics::ColliderDescription& desc)
{
    Game::Entity const bodyEntity = ResolveBodyEntity(world, entity);
    if (bodyEntity == Game::Entity::Invalid())
        return false;

    uint64_t const userData = (uint64_t)Ids::Id64(entity);

    if (actor.body != bodyEntity)
    {
        Physics::ShapeId stale(actor.shapeId);
        if (ShapeBelongsTo(stale, entity))
        {
            Physics::ActorContext::RemoveShape(stale);
            // the shape is gone, so the handle names nothing until the shape is built on the new actor
            actor.shapeId = Ids::InvalidId32;
        }
        actor.body = bodyEntity;
    }

    Physics::ActorId const actorId = (bodyEntity == entity)
        ? GetValidateActorId(entity, actor)
        : ResolveActorHandle(world, bodyEntity);
    if (actorId.id == Ids::InvalidId32)
    {
        // the body of this entity is rebuilt by its own pass in the same queue, which may not have run
        // yet. Stay queued and try again
        return true;
    }

    // the shape pose is the collider offset composed with the entity local transforms from the body down to the collider
    if (entity != bodyEntity)
    {
        Game::Entity current = entity;
        while (current != bodyEntity && current != Game::Entity::Invalid())
        {
            if (!world->HasComponent<Game::HTransform>(current))
                break;

            Game::HTransform const& ht = world->GetComponent<Game::HTransform>(current);
            Math::transform const entityLocal(ht.localPosition, ht.localOrientation);
            desc.transform = entityLocal * desc.transform;
            current = ht.parent;
        }
    }

    desc.userData = userData;

    Physics::ShapeId shape(actor.shapeId);
    if (!ShapeBelongsTo(shape, entity))
        shape = FindShapeForEntity(actorId, entity);

    if (!Physics::ActorContext::IsValidShape(shape))
    {
        Physics::ShapeId const id = Physics::ActorContext::AddShape(actorId, desc);
        if (!Physics::ActorContext::IsValidShape(id))
        {
            // the geometry could not be built, most likely a resource which has not arrived yet or
            // a wrong path. Nothing polls for it, whoever streams the content pings this entity when
            // the resource is ready
            n_warning("PhysicsManager: collider geometry '%s' could not be built, it is either still streaming or the path is wrong\n", desc.resource.Value());
            return false;
        }
        shape = id;
    }
    else if (!Physics::ActorContext::UpdateShape(shape, desc))
    {
        n_warning("PhysicsManager: collider geometry '%s' could not be rebuilt, it is either still streaming or the path is wrong\n", desc.resource.Value());
        return false;
    }

    if (shape.id != actor.shapeId)
    {
        actor.shapeId = shape.id;
        world->SetComponent(entity, actor);
    }

    return false;
}

//------------------------------------------------------------------------------
/**
*/
static bool
BuildBoxCollider(Game::World* world, Game::Entity entity, PhysicsFeature::PhysicsActor& actor)
{
    PhysicsFeature::BoxCollider collider = world->GetComponent<PhysicsFeature::BoxCollider>(entity);

    Physics::ColliderDescription desc;
    desc.type = Physics::ColliderType_Box;
    desc.materialId = Physics::LookupMaterial(collider.material);
    desc.isTrigger = collider.isTrigger;
    desc.transform = Math::transform(collider.localPosition, collider.localOrientation);
    desc.boxExtents = collider.extents;

    return BuildCollider(world, entity, actor, desc);
}

//------------------------------------------------------------------------------
/**
*/
static bool
BuildSphereCollider(Game::World* world, Game::Entity entity, PhysicsFeature::PhysicsActor& actor)
{
    PhysicsFeature::SphereCollider collider = world->GetComponent<PhysicsFeature::SphereCollider>(entity);

    Physics::ColliderDescription desc;
    desc.type = Physics::ColliderType_Sphere;
    desc.materialId = Physics::LookupMaterial(collider.material);
    desc.isTrigger = collider.isTrigger;
    desc.transform = Math::transform(collider.localPosition, collider.localOrientation);
    desc.sphereRadius = collider.radius;

    return BuildCollider(world, entity, actor, desc);
}

//------------------------------------------------------------------------------
/**
*/
static bool
BuildCapsuleCollider(Game::World* world, Game::Entity entity, PhysicsFeature::PhysicsActor& actor)
{
    PhysicsFeature::CapsuleCollider collider = world->GetComponent<PhysicsFeature::CapsuleCollider>(entity);

    Physics::ColliderDescription desc;
    desc.type = Physics::ColliderType_Capsule;
    desc.materialId = Physics::LookupMaterial(collider.material);
    desc.isTrigger = collider.isTrigger;
    desc.transform = Math::transform(collider.localPosition, collider.localOrientation);
    desc.capsuleRadius = collider.radius;
    desc.capsuleHalfHeight = collider.halfHeight;

    return BuildCollider(world, entity, actor, desc);
}

//------------------------------------------------------------------------------
/**
*/
static bool
BuildPlaneCollider(Game::World* world, Game::Entity entity, PhysicsFeature::PhysicsActor& actor)
{
    PhysicsFeature::PlaneCollider collider = world->GetComponent<PhysicsFeature::PlaneCollider>(entity);

    Physics::ColliderDescription desc;
    desc.type = Physics::ColliderType_Plane;
    desc.materialId = Physics::LookupMaterial(collider.material);
    desc.isTrigger = collider.isTrigger;
    desc.transform = Math::transform(collider.localPosition, collider.localOrientation);

    return BuildCollider(world, entity, actor, desc);
}

//------------------------------------------------------------------------------
/**
*/
static bool
BuildMeshCollider(Game::World* world, Game::Entity entity, PhysicsFeature::PhysicsActor& actor)
{
    PhysicsFeature::MeshCollider collider = world->GetComponent<PhysicsFeature::MeshCollider>(entity);

    Physics::ColliderDescription desc;
    desc.type = Physics::ColliderType_Mesh;
    desc.materialId = Physics::LookupMaterial(collider.material);
    desc.isTrigger = collider.isTrigger;
    desc.transform = Math::transform(collider.localPosition, collider.localOrientation);
    desc.resource = collider.resource;
    desc.primGroup = collider.primGroup;
    desc.topology = (Physics::MeshTopology)collider.topology;

    return BuildCollider(world, entity, actor, desc);
}

//------------------------------------------------------------------------------
/**
*/
static bool
BuildHeightFieldCollider(Game::World* world, Game::Entity entity, PhysicsFeature::PhysicsActor& actor)
{
    PhysicsFeature::HeightFieldCollider collider = world->GetComponent<PhysicsFeature::HeightFieldCollider>(entity);

    Physics::ColliderDescription desc;
    desc.type = Physics::ColliderType_HeightField;
    desc.materialId = Physics::LookupMaterial(collider.material);
    desc.isTrigger = collider.isTrigger;
    desc.transform = Math::transform(collider.localPosition, collider.localOrientation);
    desc.resource = collider.resource;
    desc.heightRange = collider.heightRange;
    desc.targetSizeX = collider.targetSizeX;
    desc.targetSizeY = collider.targetSizeY;

    return BuildCollider(world, entity, actor, desc);
}

//------------------------------------------------------------------------------
/**
    Pushes the transform of the entity onto its actor. Only needed for bodies which are not pushed by
    the poll processors, which is everything that rests where it was put.
*/
static void
PushActorPose(Game::World* world, Game::Entity entity, Physics::ActorId actorId)
{
    Physics::ActorContext::SetPositionOrientation(
        actorId, world->GetComponent<Game::Position>(entity), world->GetComponent<Game::Orientation>(entity));
}

//------------------------------------------------------------------------------
/**
    Creates, updates and releases physx actors and shapes for one entity, according to what is queued
    for it. Returns true if a shape build needs another pass.
*/
static bool
RebuildPhysicsEntity(Game::World* world, Game::Entity entity, PhysicsFeature::PhysicsActor& actor, uint32_t const pending)
{
    bool const isBody = world->HasComponent<PhysicsFeature::PhysicsResource>(entity)
        || world->HasComponent<PhysicsFeature::Rigidbody>(entity)
        || world->HasComponent<PhysicsFeature::StaticBody>(entity);

    bool retry = false;

    if (pending & (PendingBody | PendingShape | PendingStructural))
    {
        if (world->HasComponent<PhysicsFeature::PhysicsResource>(entity))
        {
            BuildActorFromPhysicsResource(world, entity, actor);
        }
        else if (world->HasComponent<PhysicsFeature::Rigidbody>(entity))
        {
            BuildRigidbody(world, entity, actor);
        }
        else if (world->HasComponent<PhysicsFeature::StaticBody>(entity))
        {
            BuildStaticBody(world, entity, actor);
        }
        else
        {
            // the entity has no body component anymore, so its actor is released, which also releases
            // every shape attached to it. The component is kept around as an empty handle, and adding a
            // body component back builds a new actor
            Physics::ActorId const current = GetValidateActorId(entity, actor);
            if (current.id != Ids::InvalidId32)
            {
                MarkOwnedShapes(world, current);
                Physics::DestroyActorInstance(current);
                actor.actorId = Ids::InvalidId32;
                actor.body = Game::Entity::Invalid();
            }
        }
    }

    if ((pending & PendingPose) && isBody)
    {
        Physics::ActorId const ownActor = GetValidateActorId(entity, actor);
        if (ownActor.id != Ids::InvalidId32)
            PushActorPose(world, entity, ownActor);
    }

    if ((pending & PendingShape) != 0 || ((pending & PendingPose) != 0 && !isBody))
    {
        if (world->HasComponent<PhysicsFeature::BoxCollider>(entity))
            retry = BuildBoxCollider(world, entity, actor) || retry;
        if (world->HasComponent<PhysicsFeature::SphereCollider>(entity))
            retry = BuildSphereCollider(world, entity, actor) || retry;
        if (world->HasComponent<PhysicsFeature::CapsuleCollider>(entity))
            retry = BuildCapsuleCollider(world, entity, actor) || retry;
        if (world->HasComponent<PhysicsFeature::PlaneCollider>(entity))
            retry = BuildPlaneCollider(world, entity, actor) || retry;
        if (world->HasComponent<PhysicsFeature::MeshCollider>(entity))
            retry = BuildMeshCollider(world, entity, actor) || retry;
        if (world->HasComponent<PhysicsFeature::HeightFieldCollider>(entity))
            retry = BuildHeightFieldCollider(world, entity, actor) || retry;
    }

    // releasing shapes whose collider component is gone walks every shape of the actor, so it is only
    // done for entities which were marked as having gained or lost a collider
    if (pending & PendingStructural)
    {
        Physics::ActorId const ownActor = GetValidateActorId(entity, actor);
        if (ownActor.id != Ids::InvalidId32)
            GarbageCollectShapes(world, entity, ownActor);
    }

    return retry;
}

//------------------------------------------------------------------------------
/**
    Rebuilds everything which has been queued since the last frame, and nothing else.
*/
void
PhysicsManager::RebuildQueued()
{
    if (this->rebuildQueue.IsEmpty())
        return;

    Game::World* world = Game::GetWorld(WORLD_DEFAULT);

    // the queue is walked as a copy, because rebuilding an entity queues other entities: the owners of
    // the shapes of an actor which is released, and the entity itself while the actor of the body its
    // shape hangs off is not built yet
    Util::Array<Game::Entity> queue;
    queue = this->rebuildQueue;
    this->rebuildQueue.Clear();

    for (Game::Entity const entity : queue)
    {
        if (!world->IsValid(entity) || !world->HasComponent<PhysicsFeature::PhysicsActor>(entity))
            continue;

        PhysicsFeature::PhysicsActor actor = world->GetComponent<PhysicsFeature::PhysicsActor>(entity);
        if (actor.pending == 0)
            continue;

        uint32_t const pending = actor.pending;

        // the mark is cleared before the entity is rebuilt, so that a mark added for the same entity
        // while its rebuild runs, which is what releasing an actor does to the owners of its shapes,
        // gets the entity back into the queue
        actor.pending = 0;
        world->SetComponent(entity, actor);

        bool const retry = RebuildPhysicsEntity(world, entity, actor, pending);

        // merge what the rebuild changed in the handle with what was marked while it ran, and keep the
        // entity queued as long as anything is left to do
        PhysicsFeature::PhysicsActor after = world->GetComponent<PhysicsFeature::PhysicsActor>(entity);
        actor.pending = after.pending | (retry ? PendingShape : 0);
        if (actor.pending != 0 && after.pending == 0)
            this->rebuildQueue.Append(entity);

        world->SetComponent(entity, actor);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitPhysicsActor(Game::World*, Game::Entity entity, PhysicsFeature::PhysicsActor* actor)
{
    // a handle also starts living when an entity is copied rather than built, which is what happens
    // when the editor world is transferred to the game world. The actor and the body it points at
    // belong to the world it came from, so a handle which starts out here owns nothing, and it is
    // queued for a build from the components it arrives with
    actor->actorId = Ids::InvalidId32;
    actor->body = Game::Entity::Invalid();
    actor->shapeId = Ids::InvalidId32;
    actor->pending = PendingAll;

    n_assert(PhysicsManager::HasInstance());
    PhysicsManager::Instance()->rebuildQueue.Append(entity);
}

//------------------------------------------------------------------------------
/**
*/
static void
EnsureHandle(Game::World* world, Game::Entity entity)
{
    if (!world->HasComponent<PhysicsFeature::PhysicsActor>(entity))
        world->AddComponent<PhysicsFeature::PhysicsActor>(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitPhysicsResource(Game::World* world, Game::Entity entity, PhysicsFeature::PhysicsResource*)
{
    EnsureHandle(world, entity);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitRigidbody(Game::World* world, Game::Entity entity, PhysicsFeature::Rigidbody*)
{
    EnsureHandle(world, entity);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitStaticBody(Game::World* world, Game::Entity entity, PhysicsFeature::StaticBody*)
{
    EnsureHandle(world, entity);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitBoxCollider(Game::World* world, Game::Entity entity, PhysicsFeature::BoxCollider*)
{
    n_assert(!HasOtherCollider(world, entity, Game::GetComponentId<PhysicsFeature::BoxCollider>()));
    EnsureHandle(world, entity);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitSphereCollider(Game::World* world, Game::Entity entity, PhysicsFeature::SphereCollider*)
{
    n_assert(!HasOtherCollider(world, entity, Game::GetComponentId<PhysicsFeature::SphereCollider>()));
    EnsureHandle(world, entity);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitCapsuleCollider(Game::World* world, Game::Entity entity, PhysicsFeature::CapsuleCollider*)
{
    n_assert(!HasOtherCollider(world, entity, Game::GetComponentId<PhysicsFeature::CapsuleCollider>()));
    EnsureHandle(world, entity);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitPlaneCollider(Game::World* world, Game::Entity entity, PhysicsFeature::PlaneCollider*)
{
    n_assert(!HasOtherCollider(world, entity, Game::GetComponentId<PhysicsFeature::PlaneCollider>()));
    EnsureHandle(world, entity);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitMeshCollider(Game::World* world, Game::Entity entity, PhysicsFeature::MeshCollider*)
{
    n_assert(!HasOtherCollider(world, entity, Game::GetComponentId<PhysicsFeature::MeshCollider>()));
    EnsureHandle(world, entity);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitHeightFieldCollider(Game::World* world, Game::Entity entity, PhysicsFeature::HeightFieldCollider*)
{
    n_assert(!HasOtherCollider(world, entity, Game::GetComponentId<PhysicsFeature::HeightFieldCollider>()));
    EnsureHandle(world, entity);
}

//------------------------------------------------------------------------------
/**
    An entity whose handle goes away takes its physx actor with it. A handle which owned no actor was a
    collider belonging to another entity, and that entity is marked so that the shapes it no longer has
    a component for are released.
*/
void
PhysicsManager::OnDecay()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);

    Game::ComponentDecayBuffer const decayBuffer = world->GetDecayBuffer(Game::GetComponentId<PhysicsFeature::PhysicsActor>());

    // the decay buffer holds a copy of the whole component of every instance which decayed
    PhysicsFeature::PhysicsActor const* handles = (PhysicsFeature::PhysicsActor const*)decayBuffer.buffer;
    for (int i = 0; i < decayBuffer.size; i++)
    {
        PhysicsFeature::PhysicsActor const& handle = handles[i];

        Physics::ActorId const actorId(handle.actorId);
        if (Physics::ActorContext::IsValid(actorId))
        {
            Physics::DestroyActorInstance(actorId);
        }

        if (handle.body != Game::Entity::Invalid())
            PhysicsManager::Mark(world, handle.body, PendingStructural);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
PollPhysicsResourceTransforms(
    Game::World* world, Game::Entity entity, Game::Position& position, Game::Orientation& orientation, PhysicsFeature::PhysicsResource const&, PhysicsFeature::PhysicsActor const& handle)
{
    Physics::ActorId const actorId = GetValidateActorId(entity, handle);
    if (actorId.id != Ids::InvalidId32)
    {
        Physics::ActorContext::GetPositionOrientation(actorId, position, orientation);

        Game::TransformHierarchy* hierarchy = world->GetHierarchy();
        if (hierarchy->IsInHierarchy(entity))
        {
            hierarchy->SetWorldTransform(entity);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
PassKinematicTransforms(
    Game::World* world, Game::Entity entity, Game::Position const& position, Game::Orientation const& orientation, PhysicsFeature::PhysicsResource const&,
    PhysicsFeature::IsKinematic, PhysicsFeature::PhysicsActor const& handle)
{
    Physics::ActorId const actorId = GetValidateActorId(entity, handle);
    if (actorId.id != Ids::InvalidId32)
    {
        Physics::ActorContext::SetPositionOrientation(actorId, position, orientation);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
PollRigidbodyTransform(
    Game::World* world, Game::Entity entity, PhysicsFeature::Rigidbody const& body, Game::Position& position, Game::Orientation& orientation,
    PhysicsFeature::PhysicsActor const& handle)
{
    if (body.actorType == PhysicsFeature::ActorType::Kinematic)
        return;

    if (world->HasComponent<Game::Static>(entity))
        return;

    Physics::ActorId const actorId = GetValidateActorId(entity, handle);
    if (actorId.id == Ids::InvalidId32)
        return;

    Physics::ActorContext::GetPositionOrientation(actorId, position, orientation);

    Game::TransformHierarchy* hierarchy = world->GetHierarchy();
    if (hierarchy->IsInHierarchy(entity))
    {
        hierarchy->SetWorldTransform(entity);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
WriteKinematicRigidbody(
    Game::World* world, Game::Entity entity, PhysicsFeature::Rigidbody const& body, Game::Position const& position,
    Game::Orientation const& orientation, PhysicsFeature::PhysicsActor const& handle)
{
    if (body.actorType != PhysicsFeature::ActorType::Kinematic)
        return;

    Physics::ActorId const actorId = GetValidateActorId(entity, handle);
    if (actorId.id == Ids::InvalidId32)
        return;

    Physics::ActorContext::SetPositionOrientation(actorId, position, orientation);
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::InitPhysicsFrameEvent()
{
    Game::World* world = Game::GetWorld(WORLD_DEFAULT);
    Game::FrameEvent* event = world->GetFramePipeline().GetFrameEvent("OnPhysicsFrame");

    // PhysX only allows reads and writes to the actors before simulate() and after fetchResults(), so the order
    // inside the physics frame is: drain the queue, write the kinematic poses, step, poll the dynamic poses
    event->AddCallback(100, Game::BothPhases, [this](Game::World* world) { this->RebuildQueued(); });

    Game::ProcessorBuilder(world, "PhysicsManager.PassKinematicTransforms"_atm)
        .Excluding<Game::Static>()
        .On("OnPhysicsFrame")
        .Order(110)
        .Func(&PassKinematicTransforms)
        .Build();

    Game::ProcessorBuilder(world, "PhysicsManager.WriteKinematicRigidbody"_atm)
        .On("OnPhysicsFrame")
        .Order(115)
        .Func(&WriteKinematicRigidbody)
        .Build();

    Game::ProcessorBuilder(world, "PhysicsManager.PollPhysicsResourceTransforms"_atm)
        .Excluding<Game::Static, PhysicsFeature::IsKinematic>()
        .On("OnPhysicsFrame")
        .Order(200)
        .Func(&PollPhysicsResourceTransforms)
        .Build();

    Game::ProcessorBuilder(world, "PhysicsManager.PollRigidbodyTransform"_atm)
        .On("OnPhysicsFrame")
        .Order(210)
        .Func(&PollRigidbodyTransform)
        .Build();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::OnActivate()
{
    Game::Manager::OnActivate();

    this->InitPhysicsFrameEvent();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::OnDeactivate()
{
    Game::Manager::OnDeactivate();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysicsManager::OnCleanup(Game::World* world)
{
    n_assert(PhysicsManager::HasInstance());

    Game::FilterBuilder::FilterCreateInfo filterInfo;
    filterInfo.inclusive[0] = Game::GetComponentId<PhysicsFeature::PhysicsActor>();
    filterInfo.access[0] = Game::AccessMode::READ;
    filterInfo.numInclusive = 1;

    Game::Filter filter = Game::FilterBuilder::CreateFilter(filterInfo);
    Game::Dataset data = world->Query(filter);
    for (int v = 0; v < data.numViews; v++)
    {
        Game::Dataset::View const& view = data.views[v];
        PhysicsFeature::PhysicsActor const* handles = (PhysicsFeature::PhysicsActor const*)view.buffers[0];

        for (IndexT i = 0; i < view.numInstances; ++i)
        {
            Physics::ActorId const actorId(handles[i].actorId);
            if (Physics::ActorContext::IsValid(actorId))
                Physics::DestroyActorInstance(actorId);
        }
    }

    Game::DestroyFilter(filter);

    this->rebuildQueue.Clear();
}

} // namespace PhysicsFeature
