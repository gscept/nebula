#pragma once
//------------------------------------------------------------------------------
/**
	@class PhysX::PhysXSlider
	
	(C) 2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "physics/joints/baseslider.h"

//------------------------------------------------------------------------------
namespace PhysX
{

class PhysXSlider : public Physics::BaseSlider
{
	__DeclareClass(PhysXSlider);
public:

	/// default constructor
	PhysXSlider();
	///
	~PhysXSlider();

	///
	virtual void Setup(const Math::matrix44 & frameA, const Math::matrix44 & frameB);

	///this wont work on physx, use a universal joint
	virtual void SetAngularLimits(float low, float high) {}
	///
	virtual void SetLinearLimits(float low, float high);
	///this wont work on physx, use a universal joint
	virtual float GetAngularLowLimit() { return -1.0f; }
	///this wont work on physx, use a universal joint
	virtual float GetAngularHighLimit(){ return -1.0f; }
	///
	virtual float GetLinearLowLimit();
	///
	virtual float GetLinearHighLimit();	

	///this wont work on physx, use a universal joint
	virtual void SetAngularMotor(float targetVelocity, float maxImpulse){}
	///this wont work on physx, use a universal joint
	virtual void SetEnableAngularMotor(bool enable){}

	///
	virtual void SetLinearMotor(float targetVelocity, float maxImpulse);
	///
	virtual void SetEnableLinearMotor(bool enable) ;

	///
	virtual float GetLinearPosition();
	///
	virtual float GetAngularPosition(){ return -1.0f; }
	///this wont work on physx, use a universal joint
	virtual void SetAngularFlags(float damping, float spring){}
	///
	virtual void SetLinearFlags(float damping, float spring);
protected:
	
};
}
