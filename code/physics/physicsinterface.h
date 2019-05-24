#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::PhysicsInterface
    

    (C) 2019 Individual contributors, see AUTHORS file
*/
#include "ids/id.h"
#include "util/dictionary.h"
#include "timing/time.h"
#include "math/float2.h"
#include "math/line.h"
#include "debug/debugtimer.h"

//------------------------------------------------------------------------------

namespace Physics
{

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
    float density;
};

struct ActorId
{
    Ids::Id32 id;
};

struct Actor
{
    physx::PxActor* actor;
    ActorId id;
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
Material & GetMaterial(IndexT idx);
/// 
SizeT GetNrMaterials();
}
//------------------------------------------------------------------------------
