#pragma once
//------------------------------------------------------------------------------
/**
    Private PhysX state object

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
    physx::PxCooking * cooking;
    physx::PxPvd *pvd;
    physx::PxPvdTransport *transport;
    Util::ArrayStack<Physics::Scene, 8> activeScenes;
    Util::ArrayStack<Physics::Material, 16> materials;
    Util::Dictionary<Util::StringAtom, IndexT> materialNameTable;

    Util::Set<Ids::Id32> awakeActors;

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
    ///
    void Update(Timing::Time delta);

    /// create new empty actor
    physx::PxRigidActor* CreateActor(bool dynamic, Math::mat4 const & transform);
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
