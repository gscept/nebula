//------------------------------------------------------------------------------
//  Controller.cc
//  (C) 2012 Johannes Hirche, LTU Skelleftea
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics2/Controller.h"

namespace Physics2
{
#if (__USE_BULLET__)
	__ImplementClass(Physics2::Controller, 'PHCT', Bullet::BulletController);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics2::Controller, 'PHCT', PhysX::PhysXController);
#else
#error "Physics2::Controller not implemented"
#endif
}