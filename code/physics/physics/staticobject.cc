//------------------------------------------------------------------------------
//  staticobject.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/staticobject.h"

namespace Physics
{
#if (__USE_BULLET__)
	__ImplementClass(Physics::StaticObject, 'PHSO', Bullet::BulletStatic);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics::StaticObject, 'PHSO', PhysX::PhysXStatic);
#elif(__USE_HAVOK__)	
	__ImplementClass(Physics::StaticObject, 'PHSO', Havok::HavokStatic);
#else
#error "Physics::StaticObject not implemented"
#endif
}