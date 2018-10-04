#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::BaseSlider

    A joint (also known as constraint) connects two Actors    

    (C) 2012-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "physics/joint.h"

//------------------------------------------------------------------------------
namespace Physics
{

class BaseSlider : public Joint
{
    __DeclareAbstractClass(BaseSlider);
public:
    

    /// default constructor
	BaseSlider() {}    

	virtual void Setup(const Math::matrix44 & frameA, const Math::matrix44 & frameB ) = 0;
		
	virtual void SetAngularLimits(float low, float high) = 0;
	virtual void SetLinearLimits(float low, float high) = 0;
	virtual float GetAngularLowLimit() = 0;
	virtual float GetAngularHighLimit() = 0;
	virtual float GetLinearLowLimit() = 0;
	virtual float GetLinearHighLimit() = 0;	

	virtual void SetAngularMotor(float targetVelocity, float maxImpulse) = 0;
	virtual void SetEnableAngularMotor(bool enable) = 0;

	virtual void SetLinearMotor(float targetVelocity, float maxImpulse) = 0;
	virtual void SetEnableLinearMotor(bool enable) = 0;
	
	virtual float GetLinearPosition() = 0;
	virtual float GetAngularPosition() = 0;
	///
	virtual void SetAngularFlags(float damping, float spring) {}
	///
	virtual void SetLinearFlags(float damping, float spring) {}
};
}




   