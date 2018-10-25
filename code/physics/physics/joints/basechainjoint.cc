//------------------------------------------------------------------------------
//  basechainjoint.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "basechainjoint.h"

namespace Physics
{
__ImplementAbstractClass(Physics::BaseChainJoint, 'BCHJ', Physics::Joint);

//------------------------------------------------------------------------------
/**
*/
BaseChainJoint::BaseChainJoint():
	initialized(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
BaseChainJoint::~BaseChainJoint()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseChainJoint::AppendJoint(const Math::point& pivotInA, const Math::point& pivotInB, const Math::quaternion& targetRotation)
{
	n_error("BaseChainJoint::AppendJoint: Do not use this base class, implement and use subclass");
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseChainJoint::AppendPhysicsBody(const Ptr<PhysicsBody>& body)
{
	n_error("BaseChainJoint::AppendPhysicsBody: Do not use this base class, implement and use subclass");
}

//------------------------------------------------------------------------------
/**
*/
int 
BaseChainJoint::GetNumJoints()
{
	n_error("BaseChainJoint::GetNumJoints: Do not use this base class, implement and use subclass");
	return -1;
}

}