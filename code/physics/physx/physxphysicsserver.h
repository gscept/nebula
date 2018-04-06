#pragma once
//------------------------------------------------------------------------------
/**
    @class PhysX::PhysXServer

    The PhysX Server

    (C) 2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "core/singleton.h"
#include "physics/physicsobject.h"
#include "physics/scene.h"
#include "physics/collider.h"
#include "util/dictionary.h"
#include "timing/time.h"
#include "math/float2.h"
#include "math/line.h"
#include "resources/simpleresourcemapper.h"
#include "debug/debugtimer.h"
#include "resources/resourcemanager.h"
#include "physics/base/physicsserverbase.h"
#include "foundation/PxAllocatorCallback.h"
#include "physxvisualdebuggersdk/PvdConnection.h"
#include "PxSimulationEventCallback.h"

//------------------------------------------------------------------------------

namespace physx
{
	class PxFoundation;
	class PxProfileZoneManager;
	class PxPhysics;
	class PxCooking;
    class PxMaterial;
}

namespace PhysX
{
class Scene;
class PhysicsBody;
class Collider;
class VisualDebuggerServer;

class NebulaAllocatorCallback : public physx::PxAllocatorCallback
{
public:
	void* allocate(size_t size, const char* typeName, const char* filename,
		int line);
	void deallocate(void* ptr);
};

class PhysXServer : public Physics::BasePhysicsServer, public physx::PxSimulationEventCallback
{
	__DeclareClass(PhysXServer);
	/// this is not a threadlocal singleton, it expects the physics simulation to be threadsafe!
    __DeclareInterfaceSingleton(PhysXServer);

public:
	
    /// constructor
	PhysXServer();
    /// destructor
	~PhysXServer();

    /// initialize the physics subsystem
	virtual bool Open();
    /// close the physics subsystem
	virtual void Close();

    /// perform simulation step(s)
	virtual void Trigger();
    /// set the current physics level object
	virtual void SetScene(Physics::Scene * level);

    
	/// render a debug visualization of the level
	virtual void RenderDebug();
	///
	virtual void HandleCollisions();
    ///
    physx::PxMaterial * GetMaterial(Physics::MaterialType type);

	physx::PxFoundation * foundation;
	physx::PxProfileZoneManager * profileZoneManager;
	physx::PxPhysics * physics;
	physx::PxCooking * cooking;
	physx::PxVisualDebuggerConnection *pvd;	 

	/// these are implementations of PxSimulationEventCallback
	///
	virtual void onConstraintBreak(physx::PxConstraintInfo* constraints, physx::PxU32 count) {}
	///
	virtual void onWake(physx::PxActor** actors, physx::PxU32 count) {}
	///
	virtual void onSleep(physx::PxActor** actors, physx::PxU32 count) {}
	///
	virtual void onContact(const physx::PxContactPairHeader& pairHeader, const physx::PxContactPair* pairs, physx::PxU32 nbPairs);
	///
	virtual void onTrigger(physx::PxTriggerPair* pairs, physx::PxU32 count);
	PhysXScene * scene;
private:
    Util::FixedArray<physx::PxMaterial*> materials;
	Threading::CriticalSection critSect;
};

//------------------------------------------------------------------------------
/**
*/
inline void*
NebulaAllocatorCallback::allocate(size_t size, const char* typeName, const char* filename,	int line)
{
	return Memory::Alloc(Memory::PhysicsHeap, size);
}

//------------------------------------------------------------------------------
/**
*/
inline void
NebulaAllocatorCallback::deallocate(void* ptr)
{
	Memory::Free(Memory::PhysicsHeap, ptr);
}

//------------------------------------------------------------------------------
/**
*/
inline physx::PxMaterial *
PhysXServer::GetMaterial(Physics::MaterialType type)
{
    n_assert2(type + 1 < this->materials.Size(), "unkown material");
    return this->materials[type + 1];
}

}; // namespace PhysX
//------------------------------------------------------------------------------
