#include "foundation/stdneb.h"
#include "physicsinterface.h"
#include "physics/physxstate.h"
#include "physics/actorcontext.h"
#include "physics/utils.h"
#include "PxPhysicsAPI.h"
#include "pvd/PxPvd.h"
#include "pvd/PxPvdTransport.h"
#include "PxSimulationEventCallback.h"

using namespace physx;


namespace Physics
{
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
    this->transport = PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 100);
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
    ActorContext::actors.Reserve(1024);

    this->time = 0.0;
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
    this->awakeActors.BeginBulkAdd();
    for (physx::PxU32 i = 0; i < count; i++)
    {
        Ids::Id32 id = (Ids::Id32)actors[i]->userData;
        this->awakeActors.BulkAdd(id);
    }
    this->awakeActors.EndBulkAdd();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysxState::onSleep(physx::PxActor** actors, physx::PxU32 count)
{
    for (physx::PxU32 i = 0; i < count; i++)
    {
        Ids::Id32 id = (Ids::Id32)actors[i]->userData;
        this->awakeActors.Erase(id);
    }
}
#pragma warning(pop)

//------------------------------------------------------------------------------
/**
*/
physx::PxRigidActor*
PhysxState::CreateActor(bool dynamic, Math::mat4 const & transform)
{
    if (dynamic)
    {
        PxRigidActor * actor = this->physics->createRigidDynamic(Neb2PxTrans(transform));
        actor->setActorFlag(physx::PxActorFlag::eSEND_SLEEP_NOTIFIES, true);
        return actor;
    }
    else
    {
        return this->physics->createRigidStatic(Neb2PxTrans(transform));
    }
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
void
PhysxState::Update(Timing::Time delta)
{    
    this->time -= delta;
    // we limit the simulation to 5 frames
    this->time = Math::n_max(this->time, -5.0 * PHYSICS_RATE);
    while (this->time < 0.0)
    {
        for (auto & scene : this->activeScenes)
        {
            // simulate synchronously
            scene.scene->simulate(PHYSICS_RATE);
            scene.scene->fetchResults(true);
        }
        state.time += PHYSICS_RATE;
    }

    for (IndexT i = 0; i < this->awakeActors.Size(); i++)
    {
        Actor& actor = ActorContext::actors[Ids::Index(this->awakeActors.KeyAtIndex(i))];
        if (actor.moveCallback.IsValid())
        {
            Math::mat4 trans = Px2NebMat(static_cast<PxRigidActor*>(actor.actor)->getGlobalPose());
            actor.moveCallback(actor.id, trans);
        }
    }
}
PhysxState state;
}