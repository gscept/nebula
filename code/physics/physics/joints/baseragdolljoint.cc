//------------------------------------------------------------------------------
//  baseragdolljoint.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "baseragdolljoint.h"

namespace Physics
{
	__ImplementAbstractClass(Physics::BaseRagdollJoint, 'BRDJ', Physics::Joint);

//------------------------------------------------------------------------------
/**
*/
BaseRagdollJoint::BaseRagdollJoint():
	initialized(false),
	motorInitialized(false)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
BaseRagdollJoint::~BaseRagdollJoint()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseRagdollJoint::SetupMotors()
{
	n_error("BaseRagdollJoint::SetupMotor: Not implemented");
}

}