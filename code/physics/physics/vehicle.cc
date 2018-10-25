//------------------------------------------------------------------------------
//  vehicle.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics2/vehicle.h"

namespace Physics2
{
#if (__USE_BULLET__)
	__ImplementClass(Physics2::Vehicle, 'PHVC', Bullet::BulletVehicle);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics2::Vehicle, 'PHVC', PhysX::PhysXVehicle);
#else
#error "Physics2::Vehicle not implemented"
#endif
}