//------------------------------------------------------------------------------
//  Scene.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/scene.h"

namespace Physics
{
#if (__USE_BULLET__)
	__ImplementClass(Physics::Scene, 'PHSC', Bullet::BulletScene);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics::Scene, 'PHSC', PhysX::PhysXScene);
#elif(__USE_HAVOK__)	
	__ImplementClass(Physics::Scene, 'PHSC', Havok::HavokScene);
#else
#error "Physics::Scene not implemented"
#endif
}