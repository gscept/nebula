#pragma once
//------------------------------------------------------------------------------
/**
	@class PhysiX::PhysXPointToPoint

	A joint (also known as constraint) connects two Actors

	(C) 2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "physics/joints/basepointtopoint.h"

//------------------------------------------------------------------------------
namespace PhysX
{

class PhysXPointToPoint : public Physics::BasePointToPoint
{
	__DeclareClass(PhysXPointToPoint);
public:

	/// default constructor
	PhysXPointToPoint();
	///
	~PhysXPointToPoint();

	///
	virtual void Setup(const Math::vector & pivot);
	///
	virtual void Setup(const Math::vector & pivotA, const Math::vector & pivotB);

	///
	virtual void SetPivotA(const Math::vector & pivot);
	///
	virtual void SetPivotB(const Math::vector & pivot);

	///
	virtual Math::vector GetPivotA();
	///
	virtual Math::vector GetPivotB();

	/// 
	virtual void SetJointParams(float tau, float damping, float /* unused impulseclamp */ ) {};

protected:
	Math::vector pivotA;
	Math::vector pivotB;

};
}
