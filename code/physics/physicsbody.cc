//------------------------------------------------------------------------------
//  physicsbody.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physicsbody.h"

namespace Physics
{
#if (__USE_BULLET__)
	__ImplementClass(Physics::PhysicsBody, 'PHBO', Bullet::BulletBody);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics::PhysicsBody, 'PHBO', PhysX::PhysXBody);
#elif(__USE_HAVOK__)	
	__ImplementClass(Physics::PhysicsBody, 'PHBO', Havok::HavokBody);
#else
#error "Physics::PhysicsBody not implemented"
#endif
}