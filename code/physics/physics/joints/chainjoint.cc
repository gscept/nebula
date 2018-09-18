//------------------------------------------------------------------------------
//  chainjoint.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/joints/chainjoint.h"

namespace Physics
{
#if (__USE_BULLET__)
	__ImplementClass(Physics::ChainJoint, 'CHJT', Physics::BaseChainJoint);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics::ChainJoint, 'CHJT', Physics::BaseChainJoint);
#elif(__USE_HAVOK__)
	__ImplementClass(Physics::ChainJoint, 'CHJT', Havok::HavokChainJoint);
#else
#error "Physics::ChainJoint not implemented"
#endif
}

