//------------------------------------------------------------------------------
//  physics/physx/physxhinge.cc
//  (C) 2016-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physx/physxhinge.h"
#include "extensions/PxRevoluteJoint.h"
#include "physxutils.h"
#include "PxRigidDynamic.h"
#include "physxphysicsserver.h"

using namespace physx;

namespace PhysX
{
	__ImplementClass(PhysX::PhysXHinge, 'PXHI', Physics::BaseHinge);

//------------------------------------------------------------------------------
/**
*/

PhysXHinge::PhysXHinge()
{

}

//------------------------------------------------------------------------------
/**
*/

PhysXHinge::~PhysXHinge()
{

}


}