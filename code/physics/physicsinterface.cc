#include "foundation/stdneb.h"
#include "PxPhysicsAPI.h"
#include "physicsinterface.h"
#include "physics/utils.h"
#include "physics/actorcontext.h"
#include "math/scalar.h"
#include "timing/time.h"
#include "physics/physxstate.h"
#include "physics/streamactorpool.h"
#include "physics/streamcolliderpool.h"
#include "resources/resourceserver.h"
#include "io/assignregistry.h"

#define PHYSX_MEMORY_ALLOCATION_DEBUG false
#define PHYSX_THREADS 2

using namespace physx;
using namespace Physics;

/// Physx simulation filter
PxFilterFlags Simulationfilter(PxFilterObjectAttributes	attributes0,
	PxFilterData				filterData0,
	PxFilterObjectAttributes	attributes1,
	PxFilterData				filterData1,
	PxPairFlags&				pairFlags,
	const void*					constantBlock,
	PxU32						constantBlockSize)
{
	PxFilterFlags filterFlags = PxDefaultSimulationFilterShader(attributes0,
		filterData0, attributes1, filterData1, pairFlags, constantBlock, constantBlockSize);
	if (pairFlags & PxPairFlag::eSOLVE_CONTACT)
	{
		if (filterData0.word1 & CollisionFeedbackFull || filterData1.word1 & CollisionFeedbackFull)
		{
			pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eNOTIFY_TOUCH_PERSISTS | PxPairFlag::eNOTIFY_TOUCH_LOST | PxPairFlag::eDETECT_DISCRETE_CONTACT | PxPairFlag::eNOTIFY_CONTACT_POINTS;
		}
		else if (filterData0.word1 & CollisionSingle || filterData1.word1 & CollisionSingle)
		{
			pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND | PxPairFlag::eDETECT_DISCRETE_CONTACT | PxPairFlag::eNOTIFY_CONTACT_POINTS;
		}
	}    
	return filterFlags;
}

namespace Physics
{

//------------------------------------------------------------------------------
/**
*/
void 
Setup()
{
    state.Setup();
    Resources::ResourceServer::Instance()->RegisterStreamPool("np", Physics::StreamActorPool::RTTI);
    Resources::ResourceServer::Instance()->RegisterStreamPool("npc", Physics::StreamColliderPool::RTTI);    
    IO::AssignRegistry::Instance()->SetAssign(IO::Assign("phys","export:physics"));

    Physics::actorPool = Resources::GetStreamPool<Physics::StreamActorPool>();
    Physics::colliderPool = Resources::GetStreamPool<Physics::StreamColliderPool>();
}

//------------------------------------------------------------------------------
/**
*/
void ShutDown()
{
    PxCloseExtensions();
    state.cooking->release();
    state.physics->release();
    state.foundation->release();
}

//------------------------------------------------------------------------------
/**
*/
IndexT
CreateScene()
{
    n_assert(state.foundation);
    IndexT idx = state.activeScenes.Size();
    state.activeScenes.Append(Scene());
    Scene & scene = state.activeScenes[idx];
    scene.dispatcher = PxDefaultCpuDispatcherCreate(PHYSX_THREADS);

    PxSceneDesc sceneDesc(state.physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0.0f, -9.81f, 0.0f);
	sceneDesc.cpuDispatcher = scene.dispatcher;
	sceneDesc.filterShader = Simulationfilter;
	scene.scene = state.physics->createScene(sceneDesc);	
	scene.scene->setSimulationEventCallback(&state);	
	scene.controllerManager= PxCreateControllerManager(*scene.scene);		
#ifdef NEBULA_DEBUG
    scene.scene->setVisualizationParameter(PxVisualizationParameter::eSCALE, 1.0f);
    scene.scene->setVisualizationParameter(PxVisualizationParameter::eWORLD_AXES, 1.0f);
    scene.scene->setVisualizationParameter(PxVisualizationParameter::eBODY_AXES, 1.0f);
    scene.scene->setVisualizationParameter(PxVisualizationParameter::eCOLLISION_SHAPES, 1.0f);
	scene.scene->getScenePvdClient()->setScenePvdFlags(PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS | PxPvdSceneFlag::eTRANSMIT_CONTACTS | PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES );
#endif
    scene.physics = state.physics;
    scene.foundation = state.foundation;
    return idx;
}

//------------------------------------------------------------------------------
/**
*/
Physics::Scene &
GetScene(IndexT idx)
{
    n_assert(idx < state.activeScenes.Size());
    return state.activeScenes[idx];
}

//------------------------------------------------------------------------------
/**
*/
void RenderDebug()
{
}

//------------------------------------------------------------------------------
/**
*/
Physics::Material&
GetMaterial(IndexT idx)
{
    n_assert2(idx < state.materials.Size(), "unkown material");
    return state.materials[idx];
}

//------------------------------------------------------------------------------
/**
*/
void
SetOnSleepCallback(Util::Delegate<void(ActorId* id, SizeT num)> const& callback)
{
	state.onSleepCallback = callback;
}

//------------------------------------------------------------------------------
/**
*/
void
SetOnWakeCallback(Util::Delegate<void(ActorId* id, SizeT num)> const& callback)
{
	state.onWakeCallback = callback;
}

//------------------------------------------------------------------------------
/**
*/
IndexT 
CreateMaterial(Util::StringAtom name, float staticFriction, float dynamicFriction, float restition, float density)
{
    n_assert(state.physics);
    PxMaterial* newMat = state.physics->createMaterial(staticFriction, dynamicFriction, restition);
    state.materials.Append(Material());
    IndexT newIdx = state.materials.Size() - 1;
    Material & mat = state.materials[newIdx];
    mat.material = newMat;
    mat.density = density;
    mat.name = name;
    mat.serialId = name.StringHashCode();
    state.materialNameTable.Add(name, newIdx);
    return newIdx;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
LookupMaterial(Util::StringAtom name)
{
    n_assert(state.materialNameTable.Contains(name));
    return state.materialNameTable[name];
}

//------------------------------------------------------------------------------
/**
*/
SizeT
GetNrMaterials()
{
    return state.materials.Size();
}


//------------------------------------------------------------------------------
/**
*/
void
Update(Timing::Time delta)
{
    state.Update(delta);    
}

//------------------------------------------------------------------------------
/**
*/
ActorId
CreateActorInstance(Physics::ActorResourceId id, Math::mat4 const & trans, bool dynamic, IndexT scene)
{
    return Physics::actorPool->CreateActorInstance(id, trans, dynamic, scene);
}

//------------------------------------------------------------------------------
/**
*/
void 
DestroyActorInstance(Physics::ActorId id)
{
    Physics::actorPool->DiscardActorInstance(id);
}

}
