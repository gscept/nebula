//------------------------------------------------------------------------------
//  bulletuniversaljoint.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "physics/bullet/conversion.h"
#include "physics/bullet/bulletuniversaljoint.h"

using namespace Math;
using namespace Physics;

namespace Bullet
{
__ImplementClass(Bullet::BulletUniversalJoint, 'PBUN', Physics::BaseUniversalJoint);


//------------------------------------------------------------------------------
/**
*/
BulletUniversalJoint::BulletUniversalJoint()
{
	/// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletUniversalJoint::Setup(const Math::vector & anchor, const Math::vector & axisA, const Math::vector & axisB)
{
	n_assert(this->constraint == NULL);
	this->constraint = n_new(btUniversalConstraint(*this->rigidBody1.cast<Bullet::BulletBody>()->GetBulletBody(),*this->rigidBody2.cast<Bullet::BulletBody>()->GetBulletBody(),
		Neb2BtVector(anchor),Neb2BtVector(axisA),Neb2BtVector(axisB)));
}

//------------------------------------------------------------------------------
/**
*/
float 
BulletUniversalJoint::GetAxisAngleA()
{
	n_assert(this->constraint != NULL);
	return ((btUniversalConstraint*)this->constraint)->getAngle1();
}

//------------------------------------------------------------------------------
/**
*/
float 
BulletUniversalJoint::GetAxisAngleB()
{
	n_assert(this->constraint != NULL);
	return ((btUniversalConstraint*)this->constraint)->getAngle2();
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletUniversalJoint::SetLowLimits(float A, float B)
{
	n_assert(this->constraint != NULL);
	((btUniversalConstraint*)this->constraint)->setLowerLimit(A,B);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletUniversalJoint::SetHighLimits(float A, float B)
{
	n_assert(this->constraint != NULL);
	((btUniversalConstraint*)this->constraint)->setUpperLimit(A,B);
}
}