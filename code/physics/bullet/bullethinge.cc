//------------------------------------------------------------------------------
//  bullethinge.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "physics/bullet/conversion.h"
#include "physics/bullet/bullethinge.h"


namespace Bullet
{

using namespace Math;
using namespace Physics;

	
	__ImplementClass(Bullet::BulletHinge, 'PBHI', Physics::BaseHinge);

	
BulletHinge::BulletHinge()
{
	/// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletHinge::Setup(const Math::vector & pivot, const Math::vector & axis )
{
	n_assert(this->constraint == NULL);
	this->constraint = n_new(btHingeConstraint(*this->rigidBody1.cast<Bullet::BulletBody>()->GetBulletBody(),Neb2BtVector(pivot),Neb2BtVector(axis)));

}
	
//------------------------------------------------------------------------------
/**
*/
void 
BulletHinge::Setup(const Math::vector & pivotA, const Math::vector & axisInA, const Math::vector & pivotB, const Math::vector & axisInB)
{
	n_assert(this->constraint == NULL);
	this->constraint = n_new(btHingeConstraint(*this->rigidBody1.cast<Bullet::BulletBody>()->GetBulletBody(),*this->rigidBody2.cast<Bullet::BulletBody>()->GetBulletBody(),
		Neb2BtVector(pivotA),Neb2BtVector(axisInA),
		Neb2BtVector(pivotB),Neb2BtVector(axisInB)));
		
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletHinge::SetLimits(float low, float high)
{
	n_assert(this->constraint != NULL);
	((btHingeConstraint*)this->constraint)->setLimit(low,high);
}

//------------------------------------------------------------------------------
/**
*/
float 
BulletHinge::GetLowLimit()
{
	n_assert(this->constraint != NULL);
	return ((btHingeConstraint*)this->constraint)->getLowerLimit();
}

//------------------------------------------------------------------------------
/**
*/
float 
BulletHinge::GetHighLimit()
{
	return ((btHingeConstraint*)this->constraint)->getUpperLimit();
}

void 
BulletHinge::SetAxis(const Math::vector & axis)
{
	n_assert(this->constraint != NULL);
	btVector3 ax = Neb2BtVector(axis);
	((btHingeConstraint*)this->constraint)->setAxis(ax);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletHinge::SetAngularMotor(float targetVelocity, float maxImpulse)
{
	n_assert(this->constraint != NULL);
	((btHingeConstraint*)this->constraint)->enableAngularMotor(false, targetVelocity,maxImpulse);
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletHinge::SetEnableAngularMotor(bool enable)
{
	n_assert(this->constraint != NULL);
	((btHingeConstraint*)this->constraint)->enableMotor(enable);

}

//------------------------------------------------------------------------------
/**
*/
float 
BulletHinge::GetHingeAngle()
{
	n_assert(this->constraint != NULL);
	return ((btHingeConstraint*)this->constraint)->getHingeAngle();

}

} // namespace Bullet
