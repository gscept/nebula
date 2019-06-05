#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::PhysicsInterface
    

    (C) 2019 Individual contributors, see AUTHORS file
*/
#include "ids/id.h"
#include "timing/time.h"
#include "util/delegate.h"
#include "util/set.h"
#include "util/arraystack.h"
#include "util/stringatom.h"
#include "math/matrix44.h"
#include "resources/resourceid.h"

#include <functional>
#include "PxPhysicsAPI.h"

//------------------------------------------------------------------------------

namespace Physics
{

RESOURCE_ID_TYPE(ActorResourceId);
RESOURCE_ID_TYPE(ColliderId);

enum CollisionFeedbackFlag
{
	/// callbacks for begin, persist, and end collision
	CollisionFeedbackFull	= 1,
	/// only on first contact
	CollisionSingle			= 2
};

struct Material
{
    physx::PxMaterial * material;
    Util::StringAtom name;
    uint64_t serialId;
    // for actors created via streamactorpool this is ignored as it can have multiple materials in it
    float density;
};

struct ActorId
{
    Ids::Id32 id;
    ActorId() :id(Ids::InvalidId32) {}
    ActorId(uint32_t i) : id(i) {}
};

struct Actor
{
    physx::PxActor* actor;
    ActorId id;
    ActorResourceId res;
    uint64_t userData;
    // FIXME delegate doesnt seem to work here (see testviewer for example)
    //std::function<void(ActorId id, Math::matrix44 const&)> moveCallback;
    Util::Delegate<void(ActorId id, Math::matrix44 const&)> moveCallback;
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
};

/// initialize the physics subsystem and create a default scene
void Setup();
/// close the physics subsystem
void ShutDown();

/// perform simulation step(s)
void Update(Timing::Time delta);

///
IndexT CreateScene();

///
Physics::Scene& GetScene(IndexT idx = 0);

/// render a debug visualization of the level
void RenderDebug();
///
void HandleCollisions();

///
IndexT CreateMaterial(Util::StringAtom name, float staticFriction, float dynamicFriction, float restition, float density);
///
Material & GetMaterial(IndexT idx);
///
IndexT LookupMaterial(Util::StringAtom name);
/// 
SizeT GetNrMaterials();

///
ActorId CreateActorInstance(Physics::ActorResourceId id, Math::matrix44 const & trans, bool dynamic, IndexT scene = 0);
///
void DestroyActorInstance(Physics::ActorId id);
}
//------------------------------------------------------------------------------
