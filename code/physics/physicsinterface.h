#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::PhysicsInterface
    

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
#include "ids/id.h"
#include "ids/idallocator.h"
#include "timing/time.h"
#include "util/bitfield.h"
#include "util/color.h"
#include "util/delegate.h"
#include "util/set.h"
#include "util/arraystack.h"
#include "util/stringatom.h"
#include "math/mat4.h"
#include "math/transform.h"
#include "resources/resourceid.h"
#include "flat/physics/material.h"

#include <functional>
#include "PxPhysicsAPI.h"

//------------------------------------------------------------------------------

namespace Physics
{

RESOURCE_ID_TYPE(ActorResourceId);
RESOURCE_ID_TYPE(AggregateResourceId);
RESOURCE_ID_TYPE(PhysicsResourceId);
RESOURCE_ID_TYPE(ConstraintResourceId);

enum ActorType
{
    Static,
    Kinematic,
    Dynamic,
};

enum CharacterCollisionBits
{
    Sides	= 0, // Character is colliding to the sides.
    Up		= 1, // Character has collision above.
    Down    = 2, // Character has collision below.
    CharacterCollisionBitsMax
};

using CharacterCollision = Util::BitField<CharacterCollisionBitsMax>;

struct Material
{
    physx::PxMaterial * material;
    Util::StringAtom name;
    uint64_t serialId;
    float density;
};

struct ActorId
{
    Ids::Id32 id;
    ActorId() : id(Ids::InvalidId32) {}
    ActorId(uint32_t i) : id(i) {}
};

struct CharacterId
{
    Ids::Id32 id;
    CharacterId() : id(Ids::InvalidId32) {}
    CharacterId(uint32_t i) : id(i) {}
};

struct ConstraintId
{
    Ids::Id32 id;
    ConstraintId() : id(Ids::InvalidId32) {}
    ConstraintId(uint32_t i) : id(i) {}
};

struct AggregateId
{
    Ids::Id32 id;
    AggregateId() : id(Ids::InvalidId32) {}
    AggregateId(uint32_t i) : id(i) {}
};

struct Actor
{
    physx::PxActor* actor;
    ActorId id;
    ActorResourceId res;
    uint64_t userData;
#ifdef NEBULA_DEBUG
    Util::String debugName;
#endif
};

struct Character
{
    physx::PxController* controller;
    Physics::ColliderType type;
    CharacterId id;
    uint64_t userData;
#ifdef NEBULA_DEBUG
    Util::String debugName;
#endif
};

struct Constraint
{
    ConstraintId id;
    Ids::Id32 res;
    physx::PxJoint* joint;
    uint64_t userData;
};

struct Aggregate
{
    AggregateId id;
    AggregateResourceId res;
    Util::Array<ActorId> actors;
    Util::Array<ConstraintId> constraints;
};

using UpdateFunctionType = void (*) (const Actor&);

struct ContactEvent
{
    ActorId actor0;
    ActorId actor1;

    Math::point position;
    Math::vector normal;
    Math::vector impulse;
    float separation;
};

using EventCallbackType = void (*) (const Util::Array<ContactEvent>&);

struct CharacterCreateInfo
{
    struct CapsuleInfo
    {
        /// Radius of this capsule.
        float radius;
        /// Height of this capsule. The total height will be `height + 2 * radius`.
        float height; 
        /// If set to `false`, the capsule will climb over surfaces according to
        /// impact normal. If set to `true`, climbing will be limited according to
        /// the step offset.
        bool constrainClimbing;
    };
    struct BoxInfo
    {
        /// Half the width, height and depth of the characters AABB collider.
        Math::float3 halfExtents;
    };
    
    /// Determines the type of collider that the character will use.
    /// If set to Box, you need to fill out the CharacterCreateInfo::box information.
    /// If set to Capsule, you need to fill out the CharacterCreateInfo::capsule information.
    Physics::ColliderType type = Physics::ColliderType_Capsule;

    /// The maximum slope which the character can walk up. Defined as cosine of desired limit angle.
    float slopeLimit = 0.707f;
    
    /// Should the character slide down slopes?
    bool slideOnSlopes = false;
    
    /// The contact offset used by the controller.
	/// Specifies a skin around the object within which contacts will be generated.
	/// Use it to avoid numerical precision issues.
    float contactOffset = 0.1f;

    /// The maximum height of an obstacle which the character can climb.
    float stepOffset = 0.5f;

    /// Optional physics material to use.
    /// @see Physics::SetPhysicsMaterial
    /// @see Physics::LookupMaterial
    IndexT materialId = InvalidIndex;
    
    union {
        /// AABB information. Make sure to set type to Box if using this.
        BoxInfo box;
        /// Capsule information. Make sure to set type to Capsule if using this.
        CapsuleInfo capsule = {0.5f, 0.8, false};
    };

    /// Can be used to store application side information in the character.
    uint64_t userData = 0;
};

/// physx scene classes, foundation and physics are duplicated here for convenience
/// instead of static getters, might be removed later on
struct Scene
{    
    physx::PxFoundation *foundation;
    physx::PxPhysics *physics;
    physx::PxScene *scene;
    physx::PxControllerManager *controllerManager;
    physx::PxDefaultCpuDispatcher *dispatcher;
    UpdateFunctionType updateFunction = nullptr;
    EventCallbackType eventCallback = nullptr;
    Timing::Time time;
    bool isSimulating = false;
    Util::Array<Physics::ContactEvent> eventBuffer;
    Util::Set<Ids::Id32> modifiedActors;
};

/// initialize the physics subsystem and create a default scene
void Setup();
/// close the physics subsystem
void ShutDown();

/// perform sync simulation step(s)
void Update(Timing::Time delta);

/// explicit calls to simulate and fetch results. Do not mix with Update!
void BeginSimulating(Timing::Time delta, IndexT scene);
void EndSimulating(IndexT scene);

/// this will block until simulation has ended for cleanups e.g.
void FlushSimulation(IndexT scene);

///
IndexT CreateScene();
///
void DestroyScene(IndexT scene);

///
Physics::Scene& GetScene(IndexT idx = 0);

///
void SetActiveActorCallback(UpdateFunctionType callback, IndexT sceneId = 0);
///
void SetEventCallback(EventCallbackType callback, IndexT sceneId = 0);


/// render a debug visualization of the level
void RenderDebug();

/// 
void SetOnSleepCallback(Util::Delegate<void(ActorId* id, SizeT num)> const& callback);
///
void SetOnWakeCallback(Util::Delegate<void(ActorId* id, SizeT num)> const& callback);

///
IndexT SetPhysicsMaterial(Util::StringAtom name, float staticFriction, float dynamicFriction, float restition, float density);
///
Material & GetMaterial(IndexT idx);
///
IndexT LookupMaterial(Util::StringAtom name);
/// 
SizeT GetNrMaterials();

///
ActorId CreateActorInstance(Physics::ActorResourceId id, Math::transform const& trans, Physics::ActorType type, uint64_t userData, IndexT scene = 0);
///
void DestroyActorInstance(Physics::ActorId id);

}
//------------------------------------------------------------------------------
