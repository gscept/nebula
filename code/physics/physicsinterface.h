#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::PhysicsInterface
    

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
#include "ids/id.h"
#include "timing/time.h"
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
