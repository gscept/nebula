//------------------------------------------------------------------------------
//  physicsmesh.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/resource/physicsmesh.h"

namespace Physics
{
#if (__USE_BULLET__)
	__ImplementClass(Physics::PhysicsMesh, 'PHME', Bullet::BulletPhysicsMesh);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics::PhysicsMesh, 'PHME', PhysX::PhysXPhysicsMesh);
#elif(__USE_HAVOK__)
	__ImplementClass(Physics::PhysicsMesh, 'PHME', Havok::HavokPhysicsMesh);
#else
#error "Physics::PhysicsMesh not implemented"
#endif
}