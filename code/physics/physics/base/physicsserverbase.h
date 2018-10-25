#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::BasePhysicsServer

    The physics subsystem server object base class.

    (C) 2013-2018 Individual contributors, see AUTHORS file
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

//------------------------------------------------------------------------------

namespace Physics
{
class Scene;
class PhysicsBody;
class Collider;
class VisualDebuggerServer;

class CollisionReceiver
{
public:
	virtual bool OnCollision(const Ptr<PhysicsObject> & receiver, const Ptr<PhysicsObject> & collidedWith, const Ptr<Contact> & c) = 0;
};

class BasePhysicsServer : public Core::RefCounted
{
	__DeclareClass(BasePhysicsServer);
	/// this is not a threadlocal singleton, it expects the physics simulation to be threadsafe!
    __DeclareInterfaceSingleton(BasePhysicsServer);

public:
	
    /// constructor
	BasePhysicsServer();
    /// destructor
	~BasePhysicsServer();

    /// initialize the physics subsystem
	virtual bool Open();
    /// close the physics subsystem
	virtual void Close();
    /// Is server open?
	bool IsOpen() const;
    /// perform simulation step(s)
	virtual void Trigger();
    /// set the current physics level object
	virtual void SetScene(Scene * level);
    /// get the current physics level object
	const Ptr<Scene> & GetScene();
    /// access to global embedded mouse gripper
    //MouseGripper* GetMouseGripper() const;
    /// set the current simulation time
	void SetTime(Timing::Time t);
    /// get the current simulation time
	Timing::Time GetTime() const;
    
	/// render a debug visualization of the level
	virtual void RenderDebug();

	void AddFilterSet(const Util::String & name, FilterSet *);
	FilterSet* GetFilterSet(const Util::String &name);

	/// set simulation frame duration
	void SetSimulationFrameTime(float duration);
	/// set simulation frame duration
	float GetSimulationFrameTime() const;
	/// set max simulation steps per update, 0 for variable timestep
	void SetSimulationStepLimit(int steps);
	/// get max simulation steps per update
	int GetSimulationStepLimit() const;

    /// get a unique stamp value
    static uint GetUniqueStamp();

	void AddCollisionReceiver(CollisionReceiver * rec);
	void RemoveCollisionReceiver(CollisionReceiver * rec);		

	/// set whether the visualdebugger is supposed to be created in Open()
	void SetInitVisualDebuggerFlag(bool flag);
	/// get whether the visualdebugger is supposed to be created in Open()
	bool GetInitVisualDebuggerFlag() const;

	virtual void HandleCollisions() = 0;

protected:
	
   
    static uint UniqueStamp;  
    bool isOpen;
    Timing::Time time;
	float simulationFrameTime;	//< how much time that will be simulated each step (set to 1.0f/60.0f by default)
	int subSteps; // defaults to 1
	Ptr<Resources::ResourceManager> resourceManager;
    Ptr<Resources::SimpleResourceMapper> meshMapper;
	Ptr<Resources::SimpleResourceMapper> modelMapper;
    Ptr<Scene> curScene;
    
    Util::Dictionary<Util::String, FilterSet*> filterSets;
	Util::Array<CollisionReceiver*> receivers;

	bool initVisualDebugger; //< the visualdebugger will not be initialized if this is false when Open() is called (is true by default)
	Ptr<VisualDebuggerServer> visualDebuggerServer;

	_declare_timer(PhysicsTrigger)
};

//------------------------------------------------------------------------------
/**
*/
inline
uint
BasePhysicsServer::GetUniqueStamp()
{
    return ++UniqueStamp;
}

//------------------------------------------------------------------------------
/**
*/
inline
bool
BasePhysicsServer::IsOpen() const
{
    return isOpen;
}

//------------------------------------------------------------------------------
/**
    Set the current time. Should be called before each call to Trigger().

    @param  t   the current time in seconds
*/
inline
void
BasePhysicsServer::SetTime(Timing::Time t)
{
    this->time = t;
}

//------------------------------------------------------------------------------
/**
    Get the current time.

    @return     current time
*/
inline
Timing::Time
BasePhysicsServer::GetTime() const
{
	return this->time;	
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BasePhysicsServer::SetSimulationFrameTime(float duration)
{
	this->simulationFrameTime = duration;
}

//------------------------------------------------------------------------------
/**
*/
inline float 
BasePhysicsServer::GetSimulationFrameTime() const
{
	return this->simulationFrameTime;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
BasePhysicsServer::SetSimulationStepLimit(int steps)
{
	this->subSteps = steps;
}

//------------------------------------------------------------------------------
/**
*/
inline int
BasePhysicsServer::GetSimulationStepLimit() const
{
	return this->subSteps;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BasePhysicsServer::SetInitVisualDebuggerFlag(bool flag)
{
	// must be called before Open()!
	n_assert(!this->IsOpen());
	this->initVisualDebugger = flag;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
BasePhysicsServer::GetInitVisualDebuggerFlag() const
{
	return this->initVisualDebugger;
}

}; // namespace Physics
//------------------------------------------------------------------------------
