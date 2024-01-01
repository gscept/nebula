#pragma once
//------------------------------------------------------------------------------
/**
    Private PhysX state object

    @copyright
    (C) 2019-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------

#include "physicsinterface.h"
#include "physics/callbacks.h"
#include "ids/idgenerationpool.h"
#include "util/set.h"

#define MAX_SHAPE_OVERLAPS 256

// we aim for 60hz physics
#define PHYSICS_RATE 0.016

namespace Physics
{

class PhysxState : public physx::PxSimulationEventCallback
{
public:
    physx::PxFoundation * foundation;
    physx::PxPhysics * physics;
    physx::PxPvd *pvd;
    physx::PxPvdTransport *transport;
    Util::StackArray<Physics::Scene, 8> activeScenes;
    Util::StackArray<Physics::Material, 16> materials;
    Util::Dictionary<Util::StringAtom, IndexT> materialNameTable;
    Util::StackArray<IndexT, 8> activeSceneIds;
    Util::StackArray<IndexT, 8> deadSceneIds;

    Physics::Material FallbackMaterial;

    Util::Set<Ids::Id32> awakeActors;

    Physics::Allocator allocator;
    Physics::ErrorCallback errorCallback;
    physx::PxOverlapHit overlapBuffer[MAX_SHAPE_OVERLAPS];

    Util::Delegate<void(ActorId*, SizeT)> onSleepCallback;
    Util::Delegate<void(ActorId*, SizeT)> onWakeCallback;

    /// 
    PhysxState();
    /// Setup Px subsystems
    void Setup();
    ///
    void Shutdown();

    /// convenience function to do a sync full simulation step. Do not mix Update and Begin/EndFrame!
    void Update(Timing::Time delta);

    /// explicit call to simulate, will process async in the background
    void BeginSimulating(Timing::Time delta, IndexT scene);
    /// explicit call to fetch the results of the simulation
    void EndSimulating(IndexT scene);

    /// will block until a potential simulation step is done
    void FlushSimulation(IndexT scene);

    /// create new empty actor
    physx::PxRigidActor* CreateActor(ActorType type, Math::mat4 const & transform);
    /// create new empty actor
    physx::PxRigidActor* CreateActor(ActorType type, Math::vec3 const& location, Math::quat const& orientation);
    /// deregister an actor
    void DiscardActor(ActorId id);

    /// connect to an instance of pvd
    void ConnectPVD();
    /// disconnect from pvd again
    void DisconnectPVD();

    /// these are implementations of PxSimulationEventCallback
    ///
    void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) {}
    ///
    void onWake(physx::PxActor** actors, physx::PxU32 count);
    ///
    void onSleep(physx::PxActor** actors, physx::PxU32 count);
    ///
    void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs) {}
    ///
    void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count) {}
    ///

    void onAdvance(const physx::PxRigidBody*const* bodyBuffer, const physx::PxTransform* poseBuffer, const physx::PxU32 count) {}
};

extern PhysxState state;
}
