#pragma once
//------------------------------------------------------------------------------
/**
	@class PhysiX::PhysXHinge
	

	(C) 2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "physics/joints/basehinge.h"

//------------------------------------------------------------------------------
namespace PhysX
{

class PhysXHinge : public Physics::BaseHinge
{
	__DeclareClass(PhysXHinge);
public:

	/// default constructor
	PhysXHinge();
	///
	~PhysXHinge();	

	// FIXME none of these are implmented

	virtual void Setup(const Math::vector & pivot, const Math::vector & axis) {}
	virtual void Setup(const Math::vector & pivotA, const Math::vector & axisInA, const Math::vector & pivotB, const Math::vector & axisInB){}

	virtual void SetLimits(float low, float high){}
	virtual float GetLowLimit() { return -1.0f; }
	virtual float GetHighLimit() { return -1.0f; }

	virtual void SetAxis(const Math::vector & axis) {}

	virtual void SetAngularMotor(float targetVelocity, float maxImpulse) {}
	virtual void SetEnableAngularMotor(bool enable) {}

	virtual float GetHingeAngle() { return -1.0f; }
};
}
