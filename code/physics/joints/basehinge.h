#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::BaseHinge

    A joint (also known as constraint) connects two Actors    

    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "physics/joint.h"

//------------------------------------------------------------------------------
namespace Physics
{

class BaseHinge : public Joint
{
    __DeclareAbstractClass(BaseHinge);
public:
    

    /// default constructor
	BaseHinge() {}    

	virtual void Setup(const Math::vector & pivot, const Math::vector & axis ) = 0;
	virtual void Setup(const Math::vector & pivotA, const Math::vector & axisInA, const Math::vector & pivotB, const Math::vector & axisInB) = 0;
	
	virtual void SetLimits(float low, float high) = 0;
	virtual float GetLowLimit() = 0;
	virtual float GetHighLimit() = 0;

	virtual void SetAxis(const Math::vector & axis) = 0;

	virtual void SetAngularMotor(float targetVelocity, float maxImpulse) = 0;
	virtual void SetEnableAngularMotor(bool enable) = 0;
	
	virtual float GetHingeAngle() = 0;
};
}




   