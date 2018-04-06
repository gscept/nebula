#pragma once
//------------------------------------------------------------------------------
/**
@class Bullet::BulletHinge

(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "physics/joints/basehinge.h"
#include "physics/base/basescene.h"

class btRigidBody;
class btDynamicsWorld;
class btTypedConstraint;

namespace Bullet
{

class BulletHinge: public Physics::BaseHinge
{
	__DeclareClass(BulletHinge);

public:
	/// default constructor
	BulletHinge();

	void Setup(const Math::vector & pivot, const Math::vector & axis );
	void Setup(const Math::vector & pivotA, const Math::vector & axisInA, const Math::vector & pivotB, const Math::vector & axisInB);


	void SetLimits(float low, float high) ;
	float GetLowLimit() ;
	float GetHighLimit();

	void SetAxis(const Math::vector & axis) ;

	void SetAngularMotor(float targetVelocity, float maxImpulse) ;
	void SetEnableAngularMotor(bool enable) ;

	float GetHingeAngle() ;
};

}
