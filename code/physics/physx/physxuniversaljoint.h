#pragma once
//------------------------------------------------------------------------------
/**
	@class PhysiX::PhysXPointToPoint

	A joint (also known as constraint) connects two Actors

	(C) 2016-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "physics/joints/baseuniversaljoint.h"

//------------------------------------------------------------------------------
namespace PhysX
{

class PhysXUniversalJoint: public Physics::BaseUniversalJoint
{
	__DeclareClass(PhysXUniversalJoint);
public:

	/// default constructor
	PhysXUniversalJoint();
	///
	~PhysXUniversalJoint();

	// FIXME none of these are implemented

	virtual void Setup(const Math::vector & anchor, const Math::vector & axisA, const Math::vector & axisB) {};

	virtual float GetAxisAngleA() { return -1.0f; }
	virtual float GetAxisAngleB() { return -1.0f; }

	virtual void SetLowLimits(float A, float B) {};
	virtual void SetHighLimits(float A, float B) {};

};
}
