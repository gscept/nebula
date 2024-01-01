//------------------------------------------------------------------------------
//  physxstate.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "PxConfig.h"
#include "physicsinterface.h"
#include "physics/physxstate.h"
#include "physics/actorcontext.h"
#include "physics/utils.h"
#include "PxPhysicsAPI.h"
#include "pvd/PxPvd.h"
#include "input/inputserver.h"
#include "input/keyboard.h"
#include "pvd/PxPvdTransport.h"
#include "PxSimulationEventCallback.h"
#include "profiling/profiling.h"

using namespace physx;

namespace Physics
{
//------------------------------------------------------------------------------
/**
*/
PhysxState::PhysxState() : foundation(nullptr), physics(nullptr), pvd(nullptr), transport(nullptr)
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

    this->physics = PxCreatePhysics(PX_PHYSICS_VERSION, *this->foundation, PxTolerancesScale(), false, this->pvd);
    n_assert2(this->physics, "PxCreatePhysics failed!");

    if (!PxInitExtensions(*this->physics, this->pvd))
    {
        n_error("PxInitExtensions failed!");
    }

    // preallocate actors
    ActorContext::actors.Reserve(1024);
}


//------------------------------------------------------------------------------
/**
*/
void PhysxState::Shutdown()
{
   
    PxCloseExtensions();
    this->physics->release();
    this->physics = nullptr;
    if (this->pvd != nullptr)
    {
        this->DisconnectPVD();
        this->pvd->release();
        this->pvd = nullptr;
    }
    this->foundation->release();
    this->foundation = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysxState::ConnectPVD()
{
    if (!this->pvd->isConnected())
    {
        if (this->transport == nullptr)
        {
            this->transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 100);
        }
        this->pvd->connect(*this->transport, PxPvdInstrumentationFlag::eALL);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
PhysxState::DisconnectPVD()
{
    if (pvd != nullptr && this->pvd->isConnected())
    {
        this->pvd->disconnect();
    }
}


// avoid warning about truncating the void
#pragma warning(push)
#pragma warning(disable: 4311)
//------------------------------------------------------------------------------
/**
*/
void
PhysxState::onWake(physx::PxActor** actors, physx::PxU32 count)
{
    if (this->onWakeCallback.IsValid())
    {
        Util::StackArray<ActorId, 128> actorIds;
        actorIds.Reserve(count);
        for (physx::PxU32 i = 0; i < count; i++)
        {
            Ids::Id32 id = (Ids::Id32)(int64_t)actors[i]->userData;
            actorIds.Append(id);
        }
        this->onWakeCallback(actorIds.Begin(), actorIds.Size());
    }
}

//------------------------------------------------------------------------------
/**
*/
void
PhysxState::onSleep(physx::PxActor** actors, physx::PxU32 count)
{
    if (this->onSleepCallback.IsValid())
    {
        Util::StackArray<ActorId, 128> actorIds;
        actorIds.Reserve(count);
        for (physx::PxU32 i = 0; i < count; i++)
        {
            Ids::Id32 id = (Ids::Id32)(int64_t)actors[i]->userData;
            actorIds.Append(id);
        }
        this->onSleepCallback(actorIds.Begin(), actorIds.Size());
    }
}
#pragma warning(pop)

//------------------------------------------------------------------------------
/**
*/
physx::PxRigidActor*
PhysxState::CreateActor(ActorType type, Math::mat4 const& transform)
{
    Math::vec3 outScale; Math::quat outRotation; Math::vec3 outTranslation;
    Math::decompose(transform, outScale, outRotation, outTranslation);
    n_assert2(outScale == Math::_plus1, "Cant scale physics actors");
    return CreateActor(type, outTranslation, outRotation);
}

//------------------------------------------------------------------------------
/**
*/
physx::PxRigidActor*
PhysxState::CreateActor(ActorType type, Math::vec3 const& location, Math::quat const& orientation)
{
    PxTransform pxTrans(Neb2PxVec(location), Neb2PxQuat(orientation));
    switch (type)
    {
        case ActorType::Dynamic:
        {
            PxRigidActor* actor = this->physics->createRigidDynamic(pxTrans);
            actor->setActorFlag(physx::PxActorFlag::eSEND_SLEEP_NOTIFIES, true);
            return actor;
        }
        break;
        case ActorType::Kinematic:
        {
            PxRigidDynamic* actor = this->physics->createRigidDynamic(pxTrans);
            actor->setRigidBodyFlag(PxRigidBodyFlag::eKINEMATIC, true);
            return actor;
        }
        break;
        case ActorType::Static:
            return this->physics->createRigidStatic(pxTrans);
        break;
    }
    n_error("unhandled actor type");
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void 
PhysxState::DiscardActor(ActorId id)
{
    IndexT idx = this->awakeActors.FindIndex(id.id);

    if (idx != InvalidIndex)
    {
        this->awakeActors.EraseAtIndex(idx);
    }
}

//------------------------------------------------------------------------------
/**
*/
static void CollectModified(Physics::Scene& scene, Util::Set<Ids::Id32>& modifiedActors)
{
    uint32_t activeActorCount = 0;
    PxActor** activeActors = scene.scene->getActiveActors(activeActorCount);
    for (uint32_t i = 0; i < activeActorCount; i++)
    {
        Ids::Id32 id = (Ids::Id32)(int64_t)activeActors[i]->userData;
        modifiedActors.Add(id);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
PhysxState::Update(Timing::Time delta)
{
    N_MARKER_BEGIN(Update, Physics);
    if (Input::InputServer::Instance()->GetDefaultKeyboard()->KeyDown(Input::Key::F3))
    {
        if (!this->pvd->isConnected()) this->ConnectPVD();
        else this->DisconnectPVD();
    }

    Util::Set<Ids::Id32> modifiedActors;
    for (IndexT Id : this->activeSceneIds)
    {
        modifiedActors.Clear();
        Physics::Scene& scene = this->activeScenes[Id];
        scene.time -= delta;
        // we limit the simulation to 5 frames
        scene.time = Math::max(scene.time, -5.0 * PHYSICS_RATE);
        while (scene.time < 0.0)
        {
            scene.scene->simulate(PHYSICS_RATE);
            scene.scene->fetchResults(true);
            scene.time += PHYSICS_RATE;
            if (scene.updateFunction != nullptr)
            {
                CollectModified(scene, modifiedActors);
            }
        }
        for (Ids::Id32 id : modifiedActors.KeysAsArray())
        {
            Actor& actor = ActorContext::GetActor(id);
            (*scene.updateFunction)(actor);
        }
    }
    N_MARKER_END();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysxState::BeginSimulating(Timing::Time delta, IndexT sceneId)
{
    N_MARKER_BEGIN(BeginSimulation, Physics);
#if NEBULA_DEBUG
    if (Input::InputServer::Instance()->GetDefaultKeyboard()->KeyDown(Input::Key::F3))
    {
        if (!this->pvd->isConnected()) this->ConnectPVD();
        else this->DisconnectPVD();
    }
#endif

    n_assert(this->activeSceneIds.FindIndex(sceneId) != InvalidIndex);
    Physics::Scene& scene = this->activeScenes[sceneId];
    n_assert(scene.isSimulating == false);
    scene.time -= delta;

    if (scene.time < -PHYSICS_RATE)
    {
        scene.isSimulating = scene.scene->simulate(PHYSICS_RATE);
    }
    
    N_MARKER_END();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysxState::EndSimulating(IndexT sceneId)
{
    n_assert(this->activeSceneIds.FindIndex(sceneId) != InvalidIndex);
    Physics::Scene& scene = this->activeScenes[sceneId];

    if (!scene.isSimulating)
    {
        return;
    }

    N_MARKER_BEGIN(EndSimulating, Physics);
    scene.scene->fetchResults(true);
    scene.time += PHYSICS_RATE;

    Util::Set<Ids::Id32> modifiedActors;
    if (scene.updateFunction != nullptr)
    {
        CollectModified(scene, modifiedActors);
    }

    // we limit the simulation to 5 frames
    scene.time = Math::max(scene.time, -5.0 * PHYSICS_RATE);
    while (scene.time < 0.0f)
    {
        // simulate synchronously until we are in sync again
        scene.scene->simulate(PHYSICS_RATE);
        scene.scene->fetchResults(true);
        CollectModified(scene, modifiedActors);
        scene.time += PHYSICS_RATE;
    }

    if (scene.updateFunction != nullptr)
    {
        for (Ids::Id32 id : modifiedActors.KeysAsArray())
        {
            Actor& actor = ActorContext::GetActor(id);
            (*scene.updateFunction)(actor);
        }
    }
    scene.isSimulating = false;
    N_MARKER_END();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysxState::FlushSimulation(IndexT sceneId)
{
    n_assert(this->activeSceneIds.FindIndex(sceneId) != InvalidIndex);
    Physics::Scene& scene = this->activeScenes[sceneId];
    scene.scene->fetchResults(true);
}

PhysxState state;
}
