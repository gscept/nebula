//------------------------------------------------------------------------------
//  physics/physx/physxslider.cc
//  (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physx/physxslider.h"
#include "physxutils.h"
#include "PxRigidDynamic.h"
#include "physxphysicsserver.h"
#include "extensions/PxJointLimit.h"
#include "extensions/PxPrismaticJoint.h"

using namespace physx;

namespace PhysX
{
	__ImplementClass(PhysX::PhysXSlider, 'PXSL', Physics::BaseSlider);

//------------------------------------------------------------------------------
/**
*/

PhysXSlider::PhysXSlider()
{

}

//------------------------------------------------------------------------------
/**
*/

PhysXSlider::~PhysXSlider()
{

}

//------------------------------------------------------------------------------
/**
*/
void
PhysXSlider::Setup(const Math::matrix44 & frameA, const Math::matrix44 & frameB)
{
	PxTransform fa(Neb2PxTrans(frameA));
	PxTransform fb(Neb2PxTrans(frameB));
	
	this->joint.prismatic = PxPrismaticJointCreate(*PhysXServer::Instance()->physics, this->GetBody1()->GetBody(), fa, this->GetBody2()->GetBody(), fb);	
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXSlider::SetLinearLimits(float low, float high)
{	
	PxJointLinearLimitPair limit(PxTolerancesScale(), low, high);	
	this->joint.prismatic->setLimit(limit);
}

//------------------------------------------------------------------------------
/**
*/
float
PhysXSlider::GetLinearLowLimit()
{
	return this->joint.prismatic->getLimit().lower;
}

//------------------------------------------------------------------------------
/**
*/
float
PhysXSlider::GetLinearHighLimit()
{
	return this->joint.prismatic->getLimit().upper;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXSlider::SetLinearMotor(float targetVelocity, float maxImpulse)
{
	n_error("not implemented");
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXSlider::SetEnableLinearMotor(bool enable)
{
	n_error("not implemented");
}

//------------------------------------------------------------------------------
/**
*/
float
PhysXSlider::GetLinearPosition()
{
	return this->joint.prismatic->getRelativeTransform().p.magnitude();
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXSlider::SetLinearFlags(float damping, float spring)
{
	PxJointLinearLimitPair limit = this->joint.prismatic->getLimit();
	limit.damping = damping;
	limit.stiffness = spring;
	this->joint.prismatic->setLimit(limit);
}

}