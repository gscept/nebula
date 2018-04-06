//------------------------------------------------------------------------------
//  physics/physx/physxpointtopointjoint.cc
//  (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physx/physxpointtopoint.h"
#include "extensions/PxFixedJoint.h"
#include "physxutils.h"
#include "PxRigidDynamic.h"
#include "physxphysicsserver.h"

using namespace physx;

namespace PhysX
{
	__ImplementClass(PhysX::PhysXPointToPoint, 'XPP1', Physics::BasePointToPoint);

//------------------------------------------------------------------------------
/**
*/

PhysXPointToPoint::PhysXPointToPoint()
{

}

//------------------------------------------------------------------------------
/**
*/

PhysXPointToPoint::~PhysXPointToPoint()
{

}

//------------------------------------------------------------------------------
/**
*/
void
PhysXPointToPoint::Setup(const Math::vector & pivot)
{
	PxTransform ident(PxIdentity);
	PxTransform trans(Neb2PxVec(pivot));
	this->joint.fixed = PxFixedJointCreate(*PhysXServer::Instance()->physics, this->GetBody1()->GetBody(), trans, 0, ident);
	this->pivotA = pivot;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXPointToPoint::Setup(const Math::vector & pivotA, const Math::vector & pivotB)
{
	PxTransform pa(Neb2PxVec(pivotA));
	PxTransform pb(Neb2PxVec(pivotB));
	this->joint.fixed = PxFixedJointCreate(*PhysXServer::Instance()->physics, this->GetBody1()->GetBody(), pa, this->GetBody2()->GetBody(), pb);
	this->pivotA = pivotA;
	this->pivotB = pivotB;
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXPointToPoint::SetPivotA(const Math::vector & pivot)
{
	this->pivotA = pivot;
	if (this->joint.fixed)
	{
		PxTransform trans(Neb2PxVec(pivot));
		this->joint.fixed->setLocalPose(PxJointActorIndex::eACTOR0, trans);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXPointToPoint::SetPivotB(const Math::vector & pivot)
{
	this->pivotB = pivot;
	if (this->joint.fixed)
	{		
		PxTransform trans(Neb2PxVec(pivot));
		this->joint.fixed->setLocalPose(PxJointActorIndex::eACTOR1, trans);
	}
}

//------------------------------------------------------------------------------
/**
*/
Math::vector
PhysXPointToPoint::GetPivotA()
{
	if (this->joint.fixed)
	{
		this->pivotA = Px2NebVec(this->joint.fixed->getLocalPose(PxJointActorIndex::eACTOR0).p);
	}
	return this->pivotA;
}

//------------------------------------------------------------------------------
/**
*/
Math::vector
PhysXPointToPoint::GetPivotB()
{
	if (this->joint.fixed)
	{
		this->pivotB = Px2NebVec(this->joint.fixed->getLocalPose(PxJointActorIndex::eACTOR1).p);
	}
	return this->pivotB;
}

}