//------------------------------------------------------------------------------
//  HingeJoint.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/joints/hingejoint.h"

namespace Physics
{
#if (__USE_BULLET__)
	__ImplementClass(Physics::HingeJoint, 'PHIJ', Bullet::BulletHinge);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics::HingeJoint, 'PHIJ', PhysX::PhysXHinge);
#elif(__USE_HAVOK__)
	__ImplementClass(Physics::HingeJoint, 'PHIJ', Havok::HavokHinge);
#else
#error "Physics::HingeJoint not implemented"
#endif
}

