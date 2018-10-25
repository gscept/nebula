//------------------------------------------------------------------------------
//  Joint.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/joint.h"

namespace Physics
{
#if (__USE_BULLET__)
	__ImplementClass(Physics::Joint, 'PHJT', Bullet::BulletJoint);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics::Joint, 'PHJT', PhysX::PhysXJoint);
#elif(__USE_HAVOK__)	
	__ImplementClass(Physics::Joint, 'PHJT', Havok::HavokJoint);
#else
#error "Physics::Joint not implemented"
#endif
}