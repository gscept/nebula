#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::PhysicsBody

    RigidBody is the  base class for all types of rigid bodies.
    Subclasses of RigidBody implement specific shapes. RigidBodies can
    be connected by Joints to form a hierarchy.
    
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "math/matrix44.h"
#include "math/vector.h"
#include "math/plane.h"
#include "physics/scene.h"
#include "util/fixedarray.h"

//------------------------------------------------------------------------------

namespace Physics
{

class Scene;
class Collider;
class PhysicsMesh;

class BaseRigidBody : public PhysicsObject
{
	__DeclareClass(BaseRigidBody);
public:     

    /// constructor
	BaseRigidBody();	
    /// destructor
    virtual ~BaseRigidBody();        	

    /// return true if currently attached
	bool IsAttached() const;

    /// set the body's linear velocity
	virtual void SetLinearVelocity(const Math::vector& v);
    /// get the body's linear velocity
	virtual Math::vector GetLinearVelocity() const;
    /// set the body's angular velocity
	virtual void SetAngularVelocity(const Math::vector& v);
    /// get the body's angular velocity
	virtual Math::vector GetAngularVelocity() const;

	/// set the angular factor on the rigid body
	virtual void SetAngularFactor(const Math::vector& v) = 0;
	/// get the angular factor on the rigid body
	virtual Math::vector GetAngularFactor() const = 0;

	/// set the mass of the body
	virtual void SetMass(float) = 0;
    /// get the mass of the body
	virtual float GetMass() const;
	/// get the center of mass in local space
	virtual Math::vector GetCenterOfMassLocal();
	/// get the center of mass in world space
	virtual Math::vector GetCenterOfMassWorld();

    /// reset the force and torque accumulators
	void Reset(){}

    /// deactivate body
	void SetSleeping(bool sleeping){}
    /// return body state
	bool GetSleeping(){return true;}
	


    /// transform a global point into the body's local space
	Math::vector GlobalToLocalPoint(const Math::vector& p) const{}
    /// transform a body-local point into global space
	Math::vector LocalToGlobalPoint(const Math::vector& p) const{}

	/// apply a global impulse vector at the next time step at a global position
	void ApplyImpulseAtPos(const Math::vector& impulse, const Math::point& pos, bool multByMass = false){}

    /// called before simulation step is taken
	void OnStepBefore(){}
    /// called after simulation step is taken
	void OnStepAfter(){}
    /// called before simulation takes place
	void OnFrameBefore(){}
    /// called after simulation takes place
	void OnFrameAfter(){}
    
    /// render the debug shapes
	virtual void RenderDebug();

    /// enable/disable collision for connected bodies
	void SetConnectedCollision(bool b){}
    /// get connected collision flag
	bool GetConnectedCollision() const{}
    /// set the damping flag
	void SetDampingActive(bool active){}
    /// get the damping flag
	bool GetDampingActive() const{}
    /// set angular damping factor (0.0f..1.0f)
	void SetAngularDamping(float f){}
    /// get angular damping factor (0.0f..1.0f)
	float GetAngularDamping() const{}
    /// set linear damping factor
	void SetLinearDamping(float f){}
    /// get linear damping factor
	float GetLinearDamping() const{}

    /// set whether body is influenced by gravity
	void SetEnableGravity(bool enable){}
    /// get whether body is influenced by gravity
	bool GetEnableGravity() const {}

	virtual void SetCollideCategory(unsigned int coll){}
	unsigned int GetCollideCategory() const{}	

	virtual void SetKinematic(bool)=0;
	bool GetKinematic();

	bool HasTransformChanged();
	
	

protected:        	
	friend class PhysicsObject;
    bool transformChanged;	
	virtual void SetupFromTemplate(const PhysicsCommon & templ);
};

//------------------------------------------------------------------------------
/**
*/
inline bool 
BaseRigidBody::IsAttached() const
{
	return this->attached;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
BaseRigidBody::HasTransformChanged()
{
	return this->transformChanged;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
BaseRigidBody::GetKinematic()
{
	return (this->common.bodyFlags & Physics::Kinematic) > 0;
}

//------------------------------------------------------------------------------
/**
*/
inline float
BaseRigidBody::GetMass() const
{
	return this->common.mass;
}
}; // namespace Physics

//------------------------------------------------------------------------------
