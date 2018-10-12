//------------------------------------------------------------------------------
//  physxjoint.cc
//  (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physicsbody.h"
#include "physics/physx/physxjoint.h"

namespace PhysX
{
__ImplementClass(PhysX::PhysXJoint, 'PXO2', Physics::BaseJoint);

//------------------------------------------------------------------------------
/**
*/
PhysXJoint::PhysXJoint()
{
	this->joint.joint = NULL;
}

//------------------------------------------------------------------------------
/**
*/
PhysXJoint::~PhysXJoint()
{
	// empty
}

//------------------------------------------------------------------------------
/**
    Render the debug visualization of this shape.
*/
void
PhysXJoint::RenderDebug()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXJoint::Attach(Physics::BaseScene * world)
{
	// nothing to be done here
	n_assert(!this->isAttached);	
	this->isAttached = true;		
}

//------------------------------------------------------------------------------
/**
*/
void
PhysXJoint::Detach()
{
	// nothing to be done here
	n_assert(this->isAttached);
	this->isAttached = false;
}

}