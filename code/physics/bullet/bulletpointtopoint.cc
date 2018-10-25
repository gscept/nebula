//------------------------------------------------------------------------------
//  bulletpointpoint.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "physics/bullet/conversion.h"
#include "physics/bullet/bulletpointtopoint.h"

using namespace Math;
using namespace Physics;

namespace Bullet
{
__ImplementClass(Bullet::BulletPointToPoint, 'PBPP', Physics::BasePointToPoint);


//------------------------------------------------------------------------------
/**
*/
BulletPointToPoint::BulletPointToPoint()
{
	/// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletPointToPoint::Setup(const Math::vector & pivot)
{
	n_assert(this->constraint == NULL);
	this->constraint = n_new(btPoint2PointConstraint(*this->rigidBody1.cast<Bullet::BulletBody>()->GetBulletBody(),Neb2BtVector(pivot)));

}

//------------------------------------------------------------------------------
/**
*/
void 
BulletPointToPoint::Setup(const Math::vector & pivotA, const Math::vector & pivotB)
{
	n_assert(this->constraint == NULL);
	this->constraint = n_new(btPoint2PointConstraint(*this->rigidBody1.cast<Bullet::BulletBody>()->GetBulletBody(),*this->rigidBody2.cast<Bullet::BulletBody>()->GetBulletBody(),
		Neb2BtVector(pivotA),
		Neb2BtVector(pivotB)));
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletPointToPoint::SetPivotA(const Math::vector & pivot)
{
	n_assert(this->constraint != NULL);
	((btPoint2PointConstraint*)this->constraint)->setPivotA(Neb2BtVector(pivot));
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletPointToPoint::SetPivotB(const Math::vector & pivot)
{
	n_assert(this->constraint != NULL);
	((btPoint2PointConstraint*)this->constraint)->setPivotB(Neb2BtVector(pivot));
}

//------------------------------------------------------------------------------
/**
*/
Math::vector
BulletPointToPoint::GetPivotA()
{
	n_assert(this->constraint != NULL);
	
	return Bt2NebVector(((btPoint2PointConstraint*)this->constraint)->getPivotInA());

}

//------------------------------------------------------------------------------
/**
*/
Math::vector  
BulletPointToPoint::GetPivotB()
{
	n_assert(this->constraint != NULL);

	return Bt2NebVector(((btPoint2PointConstraint*)this->constraint)->getPivotInB());

}

//------------------------------------------------------------------------------
/**
*/
void 
BulletPointToPoint::SetJointParams(float tau, float damping, float impulseclamp)
{
	n_assert(this->constraint != NULL);
	((btPoint2PointConstraint*)this->constraint)->m_setting.m_tau = tau;
	((btPoint2PointConstraint*)this->constraint)->m_setting.m_damping = damping;
	((btPoint2PointConstraint*)this->constraint)->m_setting.m_impulseClamp = impulseclamp;
}

}

