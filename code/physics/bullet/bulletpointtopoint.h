#pragma once
//------------------------------------------------------------------------------
/**
    @class Bullet::BulletPointToPoint

    (C) 2012-2018 Individual contributors, see AUTHORS file
*/
#include "physics/joints/basepointtopoint.h"
#include "physics/base/basescene.h"

class btRigidBody;
class btDynamicsWorld;
class btTypedConstraint;

namespace Bullet
{

class BulletPointToPoint: public Physics::BasePointToPoint
{
	__DeclareClass(BulletPointToPoint);

public:
	/// default constructor
	BulletPointToPoint();

	void Setup(const Math::vector & pivot);
	void Setup(const Math::vector & pivotA, const Math::vector & pivotB);

	void SetPivotA(const Math::vector & pivot);
	void SetPivotB(const Math::vector & pivot);

	Math::vector GetPivotA();
	Math::vector GetPivotB();

	void SetJointParams(float tau, float damping, float impulseclamp);

};

}
