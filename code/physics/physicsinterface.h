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

enum AxisLockBits
{
    LockLinearX  = 0, // Movement along the world X axis is locked.
    LockLinearY  = 1, // Movement along the world Y axis is locked.
    LockLinearZ  = 2, // Movement along the world Z axis is locked.
    LockAngularX = 3, // Rotation around the world X axis is locked.
    LockAngularY = 4, // Rotation around the world Y axis is locked.
    LockAngularZ = 5, // Rotation around the world Z axis is locked.
    AxisLockBitsMax
};

using AxisLock = Util::BitField<16>;

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

struct ShapeId
{
    Ids::Id32 id;
    ShapeId() : id(Ids::InvalidId32) {}
    ShapeId(uint32_t i) : id(i) {}
    const bool operator==(const ShapeId& rhs) const { return id == rhs.id; }
};

struct Actor
{
    physx::PxActor* actor;
    ActorId id;
    ActorResourceId res;

    /// false when this context only holds a record for the actor and not the actor itself, which is
    /// the case for the actor a character controller owns
    bool ownsActor = true;

    Util::Array<ShapeId> shapes;
    float mass;
    uint32_t collisionGroup;
    uint32_t feedback;
    AxisLock axisLock;
    uint64_t userData;
#ifdef NEBULA_DEBUG
    Util::String debugName;
#endif
};


struct Shape
{
    physx::PxShape* shape;
    ShapeId id;
    ActorId actor;
    IndexT materialId;
    bool isTrigger;
    /// hash of the description the current geometry was built from, used to detect changes
    uint64_t geometryHash;
    uint64_t userData;
#ifdef NEBULA_DEBUG
    Util::String debugName;
#endif
};

//------------------------------------------------------------------------------
/**
    A shape a character controller hit during its move.

    @note Only a shape hit is reported. A controller cannot name the other controller it hit as an
    entity, so controller against controller hits are not turned into an event.
*/
struct CharacterHitEvent
{
    CharacterId character;
    ActorId actor;
    ShapeId shape;

    Math::point position;
    Math::vector normal;
    float distance;
};

using CharacterHitCallbackType = void (*) (const CharacterHitEvent&);

struct Character
{
    physx::PxController* controller;
    Physics::ColliderType type;
    CharacterId id;

    /// The controller owns a physx actor internally. This context holds a record for that actor and
    /// for the shape attached to it, which is what lets a scene query name the entity the controller
    /// belongs to, and what lets the contact callbacks resolve it
    ActorId actor;
    ShapeId shape;

    uint32_t collisionGroup;
    uint32_t collisionMask;

    /// the report the create info carried, kept so the hit report can be forwarded without a lookup
    CharacterHitCallbackType reportCallback;

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

    /// word0 of the shape's filter data, which is what the simulation filter shader reads.
    /// A character with the default group 0 is accepted by every query and collides with everything.
    uint32_t collisionGroup = 0;

    /// The set of groups the controller accepts when it moves, matched against word0 of the hit
    /// shape's filter data. The controller's own group is never accepted, so a character does not
    /// collide with itself.
    uint32_t collisionMask = 0xFFFFFFFF;

    union {
        /// AABB information. Make sure to set type to Box if using this.
        BoxInfo box;
        /// Capsule information. Make sure to set type to Capsule if using this.
        CapsuleInfo capsule = {0.5f, 0.8, false};
    };

    /// Can be used to store application side information in the character.
    uint64_t userData = 0;

    /// Called for every shape the controller hits during move. The report is only generated when
    /// move is called, which is the physics frame.
    CharacterHitCallbackType reportCallback = nullptr;
};

//------------------------------------------------------------------------------
/**
    A collider description / create info. Mesh and heightfield colliders read their geometry
    through the registered Physics::GeometryLoader.

    @see    Physics::GeometryLoader
*/
struct ColliderDescription
{
    Physics::ColliderType type = Physics::ColliderType_Box;
    IndexT materialId = InvalidIndex;
    bool isTrigger = false;

    /// local transform of the shape, relative to the actor it belongs to
    Math::transform transform;

    /// shape parameters. Only the ones matching the collider type are read
    Math::vec3 boxExtents = Math::vec3(0.5f, 0.5f, 0.5f);   // half extents
    float sphereRadius = 0.5f;
    float capsuleRadius = 0.5f;
    float capsuleHalfHeight = 0.5f;

    /// mesh and heightfield geometry is read through the geometry loader, from a URN such as folder:file:msh
    Util::StringAtom resource;
    IndexT primGroup = 0;
    Physics::MeshTopology topology = Physics::MeshTopology_Triangles;
    float heightRange = 1.0f;
    float targetSizeX = 1.0f;
    float targetSizeY = 1.0f;

    uint64_t userData = 0;
};

//------------------------------------------------------------------------------
/**
    A physics body description.
*/
struct BodyDescription
{
    Physics::ActorType type = Physics::Dynamic;
    IndexT scene = 0;
    Math::transform transform;

    /// 0 means the mass is derived from the shape volumes and the material densities
    float mass = 0.0f;
    float linearDamping = 0.0f;
    float angularDamping = 0.0f;
    bool useGravity = true;
    bool ccd = false;

    uint32_t collisionGroup = 0;
    Physics::CollisionFeedback feedback = Physics::CollisionFeedback_Full;

    /// the world space axes the body is locked along or around.
    AxisLock axisLock;

    uint64_t userData = 0;
};

/// physx scene classes, foundation and physics are duplicated here for convenience
/// instead of static getters, might be removed later on
struct Scene
{    
    physx::PxFoundation *foundation = nullptr;
    physx::PxPhysics *physics = nullptr;
    physx::PxScene *scene = nullptr;
    physx::PxControllerManager *controllerManager = nullptr;
    physx::PxDefaultCpuDispatcher *dispatcher = nullptr;
    UpdateFunctionType updateFunction = nullptr;
    EventCallbackType eventCallback = nullptr;
    Util::Array<Physics::ContactEvent> eventBuffer;
    Util::Set<Ids::Id32> activeActors;
};

/// initialize the physics subsystem and create a default scene
void Setup();
/// close the physics subsystem
void ShutDown();

/// perform a sync simulation step on every scene
void Step(Timing::Time delta);
/// one sync simulation step of a single scene. Actor reads and writes have to happen outside of this
void Step(Timing::Time delta, IndexT scene);

///
IndexT CreateScene();
///
void DestroyScene(IndexT scene);

///
Physics::Scene& GetScene(IndexT idx = 0);

/// called once per step for every actor PhysX reported active on that step
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
