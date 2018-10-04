//------------------------------------------------------------------------------
//  bulletslider.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "physics/bullet/conversion.h"
#include "physics/bullet/bulletslider.h"


namespace Bullet
{

using namespace Math;
using namespace Physics;


	__ImplementClass(Bullet::BulletSlider, 'PBHI', Physics::BaseSlider);


BulletSlider::BulletSlider()
{
	/// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletSlider::Setup(const Math::matrix44 & frameA, const Math::matrix44 & frameB)
{
	n_assert(this->constraint == NULL);
	this->constraint = n_new(btSliderConstraint(*this->rigidBody1.cast<Bullet::BulletBody>()->GetBulletBody(),*this->rigidBody2.cast<Bullet::BulletBody>()->GetBulletBody(),
		Neb2BtM44Transform(frameA),
		Neb2BtM44Transform(frameB),true));

}

//------------------------------------------------------------------------------
/**
*/
void 
BulletSlider::SetAngularLimits(float low, float high)
{
	n_assert(this->constraint != NULL);
	((btSliderConstraint*)this->constraint)->setLowerAngLimit(low);
	((btSliderConstraint*)this->constraint)->setUpperAngLimit(high);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletSlider::SetLinearLimits(float low, float high)
{
	n_assert(this->constraint != NULL);
	((btSliderConstraint*)this->constraint)->setLowerLinLimit(low);
	((btSliderConstraint*)this->constraint)->setUpperLinLimit(high);
}

//------------------------------------------------------------------------------
/**
*/
float 
BulletSlider::GetAngularLowLimit()
{
	n_assert(this->constraint != NULL);
	return ((btSliderConstraint*)this->constraint)->getLowerAngLimit();
}

//------------------------------------------------------------------------------
/**
*/
float 
BulletSlider::GetAngularHighLimit()
{
	return ((btSliderConstraint*)this->constraint)->getUpperAngLimit();
}

//------------------------------------------------------------------------------
/**
*/
float 
BulletSlider::GetLinearLowLimit()
{
	n_assert(this->constraint != NULL);
	return ((btSliderConstraint*)this->constraint)->getLowerLinLimit();
}

//------------------------------------------------------------------------------
/**
*/
float 
BulletSlider::GetLinearHighLimit()
{
	return ((btSliderConstraint*)this->constraint)->getUpperLinLimit();
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletSlider::SetAngularMotor(float targetVelocity, float maxImpulse)
{
	n_assert(this->constraint != NULL);
	((btSliderConstraint*)this->constraint)->setTargetAngMotorVelocity(targetVelocity);
	((btSliderConstraint*)this->constraint)->setMaxAngMotorForce(maxImpulse);

}

//------------------------------------------------------------------------------
/**
*/
void 
BulletSlider::SetEnableAngularMotor(bool enable)
{
	n_assert(this->constraint != NULL);
	((btSliderConstraint*)this->constraint)->setPoweredAngMotor(enable);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletSlider::SetLinearMotor(float targetVelocity, float maxImpulse)
{
	n_assert(this->constraint != NULL);
	((btSliderConstraint*)this->constraint)->setTargetLinMotorVelocity(targetVelocity);
	((btSliderConstraint*)this->constraint)->setMaxLinMotorForce(maxImpulse);

}

//------------------------------------------------------------------------------
/**
*/
void 
BulletSlider::SetEnableLinearMotor(bool enable)
{
	n_assert(this->constraint != NULL);
	((btSliderConstraint*)this->constraint)->setPoweredLinMotor(enable);
}


float 
BulletSlider::GetLinearPosition()
{
	n_assert(this->constraint != NULL);
	return ((btSliderConstraint*)this->constraint)->getLinearPos();
}
//------------------------------------------------------------------------------
/**
*/
float 
BulletSlider::GetAngularPosition()
{
	n_assert(this->constraint != NULL);
	return ((btSliderConstraint*)this->constraint)->getAngularPos();
}

} // namespace Bullet