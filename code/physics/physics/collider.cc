//------------------------------------------------------------------------------
//  Shape.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/collider.h"

namespace Physics
{
#if (__USE_BULLET__)
	__ImplementClass(Physics::Collider, 'PHCO', Bullet::BulletCollider);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics::Collider, 'PHCO', PhysX::PhysXCollider);
#elif(__USE_HAVOK__)	
	__ImplementClass(Physics::Collider, 'PHCO', Havok::HavokCollider);
#else
#error "Physics::Collider not implemented"
#endif
}