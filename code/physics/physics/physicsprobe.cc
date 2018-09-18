//------------------------------------------------------------------------------
//  physicsprobe.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physicsprobe.h"

namespace Physics
{
#if (__USE_BULLET__)
	__ImplementClass(Physics::PhysicsProbe, 'PHPR', Bullet::BulletProbe);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics::PhysicsProbe, 'PHPR', PhysX::PhysXProbe);
#elif(__USE_HAVOK__)	
	__ImplementClass(Physics::PhysicsProbe, 'PHPR', Havok::HavokProbe);
#else
#error "Physics::PhysicsBody not implemented"
#endif
}