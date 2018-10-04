#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::BaseScene
        
	Base class of physics scenes (worlds)

    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "math/matrix44.h"
#include "math/vector.h"
#include "physics/physicsobject.h"
#include "physics/collider.h"
#include "../contact.h"
#include "timing/time.h"

namespace Physics
{
class PhysicsBody;
class FilterSet;
class AreaImpulse;

class BaseScene : public Core::RefCounted
{
	__DeclareClass(BaseScene);
public:

	enum RayTestType
	{
		Test_All = 0,
		Test_Closest,		

		NumRayTestTypes,
	};

	
	 /// constructor
	BaseScene();
	 /// destructor
	~BaseScene();

	/// called when the scene is attached to the physics server
	void OnActivate() {}
	/// called when the scene is detached from the physics server
	void OnDeactivate() {}
	/// perform one (or more) simulation steps depending on current time
	void Trigger() {}

    /// Render debug shapes of colliders and other objects
	virtual void RenderDebug();

		
	/// set gravity vector
	virtual void SetGravity(const Math::vector& v);
	/// get gravity vector
	const Math::vector& GetGravity() const;

	/// simple ray check from start to end with optional filter
	const Ptr<PhysicsObject> & SimpleRayCheck(const Math::vector & start, const Math::vector & end, uint objectTypes = All) {}

    /// attach a physics object to scene
	virtual void Attach(const Ptr<PhysicsObject> & obj);
    /// attach static physics object to scene (cant be moved after, can be optimized by underlying physics engine if applicable)
    virtual void AttachStatic(const Ptr<PhysicsObject> &obj);
    /// detach physics object from scene
	virtual void Detach(const Ptr<PhysicsObject> & obj);    
	
	/// return all entities within a spherical area
	virtual int GetObjectsInSphere(const Math::point& pos, float radius, const FilterSet& excludeSet, Util::Array<Ptr<PhysicsObject> >& result) {return 0;}
	/// return all entities within a box 
	virtual int GetObjectsInBox(const Math::matrix44& transform, const Math::vector& halfWidth, const Physics::FilterSet& excludeSet, Util::Array<Ptr<Physics::PhysicsObject> >& result) {return 0;}

	/// Do a ray check starting from position `pos' along ray `dir'.
	virtual Util::Array<Ptr<Contact> >RayCheck(const Math::vector& pos, const Math::vector& dir, const FilterSet& excludeSet, RayTestType rayType) = 0;
	/// do a stabbing test into the world with a ray bundle, return distance to intersection
	bool RayBundleCheck(const Math::vector& from, const Math::vector& dir, const Math::vector& upVec, const Math::vector& leftVec, float bundleRadius, const FilterSet& excludeSet, float& outContactDist);
	/// do a ray check through the mouse pointer and return closest contact
	virtual Ptr<Contact> GetClosestContactUnderMouse(const Math::line& worldMouseRay, const FilterSet& excludeSet);
	/// get closest contact along ray
	virtual Ptr<Contact> GetClosestContactAlongRay(const Math::vector& pos, const Math::vector& dir, const FilterSet& excludeSet);
	/// apply an impulse along a ray into the world onto the first object which the ray hits
	virtual bool ApplyImpulseAlongRay(const Math::vector& pos, const Math::vector& dir, const FilterSet& excludeSet, float impulse);	

	/// disable collision between two bodies
	virtual void AddIgnoreCollisionPair(const Ptr<Physics::PhysicsBody>& bodyA, const Ptr<Physics::PhysicsBody>& bodyB);

	/// get the time the world was last simulated to (can differ from PhysicsServerBase::time)
	Timing::Time GetTime();

	/// get the simulation speed
	float GetSimulationSpeed() const;
	/// get the simulation speed (must be >0)
	void SetSimulationSpeed(float speed);
	
#if 0
	void SetPointOfInterest(const Math::vector& v) {}
	/// get current point of interest
	const Math::vector& GetPointOfInterest() const {}
	/// render debug visualization


#endif

protected:
	Util::Array<Ptr<PhysicsObject> > objects;
	Timing::Time time; //< the time the world has simulated to (can differ from PhysicsServerBase::time)
	float simulationSpeed;	//< speed multiplier (defaults to 1.0)

private:
	Math::vector gravity;
};

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseScene::SetGravity(const Math::vector& v)
{
	this->gravity = v;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::vector& 
BaseScene::GetGravity() const
{
	return this->gravity;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Time 
BaseScene::GetTime()
{
	return this->time;
}

//------------------------------------------------------------------------------
/**
*/
inline float 
BaseScene::GetSimulationSpeed() const
{
	return this->simulationSpeed;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
BaseScene::SetSimulationSpeed(float speed)
{
	n_assert(speed > 0.0f);
	this->simulationSpeed = speed;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
BaseScene::AttachStatic(const Ptr<PhysicsObject> &obj)
{
    /// if not overridden just do a normal attach
    this->Attach(obj);
}
}
