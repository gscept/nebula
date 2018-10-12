#pragma once
//------------------------------------------------------------------------------
/**
	@class Bullet::BulletSlider

	(C) 2012-2018 Individual contributors, see AUTHORS file
*/
#include "physics/joints/baseslider.h"
#include "physics/base/basescene.h"

class btRigidBody;
class btDynamicsWorld;
class btTypedConstraint;

namespace Bullet
{

class BulletSlider: public Physics::BaseSlider
{
	__DeclareClass(BulletSlider);

public:
	/// default constructor
	BulletSlider();

	/// using from two matrices describing the two frames
	void Setup(const Math::matrix44 & frameA, const Math::matrix44 & frameB );

	///
	void SetAngularLimits(float low, float high);
	///
	void SetLinearLimits(float low, float high);
	///
	float GetAngularLowLimit();
	///
	float GetAngularHighLimit();
	///
	float GetLinearLowLimit();
	///
	float GetLinearHighLimit();

	///
	void SetAxis(const Math::vector & axis);

	///
	void SetAngularMotor(float targetVelocity, float maxImpulse);
	///
	void SetEnableAngularMotor(bool enable);

	///
	void SetLinearMotor(float targetVelocity, float maxImpulse);
	///
	void SetEnableLinearMotor(bool enable);

	///
	float GetLinearPosition();
	///
	float GetAngularPosition();
};

}
