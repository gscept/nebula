#pragma once
//------------------------------------------------------------------------------
/**
@class Physics2::Vehicle

    A physics Vehicle

    (C) 2012 Johannes Hirche, LTU Skelleftea
*/
#if (__USE_BULLET__)
#include "physics2/bullet/bulletVehicle.h"
namespace Physics2
{
class Vehicle : public Bullet::BulletVehicle
{
    __DeclareClass(Vehicle);     
};
}
#elif(__USE_PHYSX__)
#include "physics2/physx/physxVehicle.h"
namespace Physics2
{
class Vehicle : public PhysX::PhysXVehicle
{
	__DeclareClass(Vehicle);	  
};
}
#else
#error "Physics2::Vehicle not implemented"
#endif