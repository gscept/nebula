#pragma once
//------------------------------------------------------------------------------
/**
    @class  PhysicsFeature::PhysicsManager

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/manager.h"
#include "game/category.h"
#include "game/entity.h"
#include "game/world.h"
#include "basegamefeature/components/basegamefeature.h"
#include "physicsinterface.h"
#include "physics/actorcontext.h"
#include "resources/resourceserver.h"
#include "components/physicsfeature.h"
#include "game/api.h"

namespace PhysicsFeature
{

/// what the physics manager has queued for an entity, mixed together in the pending field of the
/// PhysicsActor component. Adding and removing components is picked up by the component inits and
/// decay callbacks, everything else is pinged by whoever changed the entity
enum PhysicsPending : uint32_t
{
    PendingPose = 1 << 0,       ///< push the pose of the actor, and the local pose of its shapes
    PendingShape = 1 << 1,      ///< build or update the shapes of the collider components
    PendingBody = 1 << 2,       ///< create or update the physx actor of the body component
    PendingStructural = 1 << 3, ///< a collider component appeared or disappeared, so shapes which no longer have one are released
    PendingAll = PendingPose | PendingShape | PendingBody | PendingStructural,
};

class PhysicsManager : public Game::Manager
{
    __DeclareClass(PhysicsManager)
    __DeclareSingleton(PhysicsManager)
public:
    PhysicsManager();
    virtual ~PhysicsManager();

    void OnActivate() override;
    void OnDeactivate() override;
    void OnFrame() override;
    void OnDecay() override;
    void OnCleanup(Game::World* world) override;

    // the PhysicsActor component is the handle which owns the physx actor of an entity. Its init
    // resets the handle and queues a full build, and it is the only place the actor and its shapes
    // are built from nothing
    static void InitPhysicsActor(Game::World*, Game::Entity, PhysicsFeature::PhysicsActor*);

    // the body and collider components hold no physx handles. Their init ensures the entity has a
    // handle, which queues its setup. Adding one of them to an entity which already has a handle is a
    // change, and changes are pinged by whoever made them, see the Mark functions below
    static void InitPhysicsResource(Game::World*, Game::Entity, PhysicsFeature::PhysicsResource*);
    static void InitRigidbody(Game::World*, Game::Entity, PhysicsFeature::Rigidbody*);
    static void InitStaticBody(Game::World*, Game::Entity, PhysicsFeature::StaticBody*);
    static void InitBoxCollider(Game::World*, Game::Entity, PhysicsFeature::BoxCollider*);
    static void InitSphereCollider(Game::World*, Game::Entity, PhysicsFeature::SphereCollider*);
    static void InitCapsuleCollider(Game::World*, Game::Entity, PhysicsFeature::CapsuleCollider*);
    static void InitPlaneCollider(Game::World*, Game::Entity, PhysicsFeature::PlaneCollider*);
    static void InitMeshCollider(Game::World*, Game::Entity, PhysicsFeature::MeshCollider*);
    static void InitHeightFieldCollider(Game::World*, Game::Entity, PhysicsFeature::HeightFieldCollider*);

    /// queues work for an entity, see PhysicsPending. An entity which is already queued keeps what is
    /// already queued, its component values are read when the queue is drained, not when it is marked
    static void Mark(Game::World*, Game::Entity, uint32_t pending);

    /// the collider components of the entity were changed, or added to an entity which already had a
    /// handle. Removing one is picked up on its own, by the decay callback. Also call this after an
    /// entity with a collider has been moved to another parent, which detaches its shape from the
    /// actor it used to hang off and attaches it to the new one
    static void MarkShapeChanged(Game::World*, Game::Entity);

    /// the body component of the entity changed: actor type, mass, damping, gravity, ccd, collision
    /// group or collision feedback. Adding or removing one is picked up on its own, by the init and
    /// the decay callback
    static void MarkBodyChanged(Game::World*, Game::Entity);

    /// the entity moved. The poll processors push and pull the pose of kinematic and dynamic bodies
    /// every frame, so this is only needed to move a static body, or a collider relative to its body
    static void MarkPoseChanged(Game::World*, Game::Entity);

    /// decay callback of the collider components, see MarkShapeChanged
    template <typename COMPONENT_TYPE>
    static void OnColliderComponentDecay(Game::World*, Game::Entity, COMPONENT_TYPE*);

    /// decay callback of the body components, which releases the physx actor of the entity
    template <typename COMPONENT_TYPE>
    static void OnBodyComponentDecay(Game::World*, Game::Entity, COMPONENT_TYPE*);

private:
    void InitPollTransformProcessor();
    void InitBuildProcessors();

    // rebuilds everything which was queued since the last frame. An entity stays queued while the
    // actor of the body its shape hangs off is not built yet. Geometry which cannot be loaded is a
    // warned failure, nothing polls for the resource, whoever streams it pings when it is ready
    void RebuildQueued();

    Util::Array<Game::Entity> rebuildQueue;
};

//------------------------------------------------------------------------------
/**
    A collider component which goes away means that the actor owning its shape has to find out that
    one of its shapes is no longer backed by a component.
*/
template <typename COMPONENT_TYPE>
inline void
PhysicsManager::OnColliderComponentDecay(Game::World* world, Game::Entity entity, COMPONENT_TYPE*)
{
    PhysicsManager::MarkShapeChanged(world, entity);
}

//------------------------------------------------------------------------------
/**
    A body component which goes away means that the entity is no longer a body, so its actor, and
    with it every shape attached to it, is released.
*/
template <typename COMPONENT_TYPE>
inline void
PhysicsManager::OnBodyComponentDecay(Game::World* world, Game::Entity entity, COMPONENT_TYPE*)
{
    PhysicsManager::Mark(world, entity, PendingBody | PendingStructural);
}

} // namespace PhysicsFeature
