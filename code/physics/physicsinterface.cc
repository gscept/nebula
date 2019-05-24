#include "foundation/stdneb.h"
#include "PxPhysicsAPI.h"
#include "pvd/PxPvd.h"
#include "pvd/PxPvdTransport.h"
#include "PxSimulationEventCallback.h"
#include "ids/idgenerationpool.h"
#include "physicsinterface.h"
#include "physics/callbacks.h"
#include "physics/utils.h"
#include "math/scalar.h"
#include "timing/time.h"

#define PHYSX_MEMORY_ALLOCATION_DEBUG false
#define PHYSX_THREADS 2
#define MAX_SHAPE_OVERLAPS 256
// we aim for 60hz physics
#define PHYSICS_RATE 0.016

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

class PhysxState : public physx::PxSimulationEventCallback
{
    public:
    physx::PxFoundation * foundation;    
    physx::PxPhysics * physics;
    physx::PxCooking * cooking;
    physx::PxPvd *pvd;
    physx::PxPvdTransport *transport;
    Util::ArrayStack<Physics::Scene,8> activeScenes;
    Util::ArrayStack<Physics::Material,16> materials;

    Util::Array<Actor> actors;
    Ids::IdGenerationPool actorPool;

    Physics::Allocator allocator;
    Physics::ErrorCallback errorCallback;
    physx::PxOverlapHit overlapBuffer[MAX_SHAPE_OVERLAPS];
    Timing::Time time;

    /// 
    PhysxState();
    /// Setup Px subsystems
    void Setup();
    ///
    void Shutdown();

    /// these are implementations of PxSimulationEventCallback
    ///
    void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) {}
    ///
    void onWake(physx::PxActor** actors, physx::PxU32 count) {}
    ///
    void onSleep(physx::PxActor** actors, physx::PxU32 count) {}
    ///
    void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs);
    ///
    void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count);
    ///
    
    void onAdvance(const PxRigidBody*const* bodyBuffer, const PxTransform* poseBuffer, const PxU32 count);
    
};



//------------------------------------------------------------------------------
/**
*/
PhysxState::PhysxState() : foundation(nullptr), physics(nullptr), cooking(nullptr), pvd(nullptr), transport(nullptr)
{
    // empty
}
//------------------------------------------------------------------------------
/**
*/
void
PhysxState::Setup()
{
    n_assert(foundation == nullptr);
    this->foundation = PxCreateFoundation(PX_PHYSICS_VERSION, this->allocator, this->errorCallback);
    n_assert2(this->foundation, "PxCreateFoundation failed!");

    this->pvd = PxCreatePvd(*this->foundation);
    this->transport = PxDefaultPvdSocketTransportCreate("127.0.0.1",5424,10);
    this->pvd->connect(*this->transport, PxPvdInstrumentationFlag::eALL);

    this->physics = PxCreatePhysics(PX_PHYSICS_VERSION, *this->foundation, PxTolerancesScale(), false, this->pvd);
    n_assert2(this->physics, "PxCreatePhysics failed!");
    
    this->cooking = PxCreateCooking(PX_PHYSICS_VERSION, *this->foundation, PxCookingParams(PxTolerancesScale()));
    n_assert2(this->cooking, "PxCreateCooking failed!");

    if (!PxInitExtensions(*this->physics, this->pvd))
    {
        n_error("PxInitExtensions failed!");
    }

    // preallocate actors
    this->actors.Reserve(1024);

    this->time = 0.0;
}



static PhysxState state;

namespace Physics
{

//------------------------------------------------------------------------------
/**
*/
void 
Setup()
{
    state.Setup();
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
Update(Timing::Time delta)
{
    state.time -= delta;
    // we limit the simulation to 5 frames
    state.time = Math::n_max(state.time, 5.0 * PHYSICS_RATE);
    while (state.time < 0.0)
    {
        for(auto & scene : state.activeScenes)
        {
            // simulate synchronously
            scene.scene->simulate(PHYSICS_RATE);
            scene.scene->fetchResults(true);
        }
        state.time += PHYSICS_RATE;
    }    
}



//------------------------------------------------------------------------------
/**
*/
static inline
PxRigidActor* 
CreateActor(PxPhysics* physics, bool dynamic, Math::matrix44 const & transform)
{
    if (dynamic)
    {
        return physics->createRigidDynamic(Neb2PxTrans(transform));
    }
    else
    {
        return physics->createRigidStatic(Neb2PxTrans(transform));
    }
}


//------------------------------------------------------------------------------
/**
*/
static ActorId
AllocateActorId(PxRigidActor* pxActor)
{
    ActorId id;
    bool reused = state.actorPool.Allocate(id.id);
    Ids::Id24 idx = Ids::Index(id.id);
    if (!reused)
    {
        state.actors.Append(Physics::Actor());
        n_assert(idx == state.actors.Size() - 1);
    }
        
    Actor& actor = state.actors[idx];
    actor.id = id;
    actor.actor = pxActor;
    return id;
}

namespace Actors
{
//------------------------------------------------------------------------------
/**
*/
ActorId
CreateBox(Math::vector const& extends, IndexT materialId, bool dynamic, Math::matrix44 const & transform, IndexT sceneId)
{
    Scene & scene = Physics::GetScene(sceneId);

    PxRigidActor* newActor = CreateActor(scene.physics, dynamic, transform);

    Material const & material = Physics::GetMaterial(materialId);
    PxShape * shape = PxRigidActorExt::createExclusiveShape(*newActor, PxBoxGeometry(Neb2PxVec(extends)), *material.material);
    if (dynamic)
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
CreateSphere(float radius, IndexT materialId, bool dynamic, Math::matrix44 const & transform, IndexT sceneId)
{
    Scene & scene = Physics::GetScene(sceneId);

    PxRigidActor* newActor = CreateActor(scene.physics, dynamic, transform);

    Material const & material = Physics::GetMaterial(materialId);
    PxShape * shape = PxRigidActorExt::createExclusiveShape(*newActor, PxSphereGeometry(radius), *material.material);
    if (dynamic)
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
CreateCapsule(float radius, float halfHeight, IndexT materialId, bool dynamic, Math::matrix44 const & transform, IndexT sceneId)
{
    Scene & scene = Physics::GetScene(sceneId);

    PxRigidActor* newActor = CreateActor(scene.physics, dynamic, transform);

    Material const & material = Physics::GetMaterial(materialId);
    PxShape * shape = PxRigidActorExt::createExclusiveShape(*newActor, PxCapsuleGeometry(radius, halfHeight), *material.material);
    if (dynamic)
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
CreatePlane(Math::plane const& plane, IndexT materialId, IndexT sceneId)
{
    Scene & scene = Physics::GetScene(sceneId);

    PxPlane pxPlane(Neb2PxVec(plane.get_normal()), Neb2PxVec(plane.get_point()));
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
GetActor(ActorId id)
{
    n_assert(state.actorPool.IsValid(id.id));
    return state.actors[Ids::Index(id.id)];
}

}

}