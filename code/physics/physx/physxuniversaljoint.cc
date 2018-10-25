//------------------------------------------------------------------------------
//  physics/physx/physxuniversaljoint.cc
//  (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physx/physxuniversaljoint.h"
#include "extensions/PxFixedJoint.h"
#include "physxutils.h"
#include "PxRigidDynamic.h"
#include "physxphysicsserver.h"

using namespace physx;

namespace PhysX
{
	__ImplementClass(PhysX::PhysXUniversalJoint, 'PXUJ', Physics::BaseUniversalJoint);

//------------------------------------------------------------------------------
/**
*/
PhysXUniversalJoint::PhysXUniversalJoint()
{

}

//------------------------------------------------------------------------------
/**
*/
PhysXUniversalJoint::~PhysXUniversalJoint()
{

}


}