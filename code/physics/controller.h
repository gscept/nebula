#pragma once
//------------------------------------------------------------------------------
/**
@class Physics2::Controller

    A physics Controller

    (C) 2012 Johannes Hirche, LTU Skelleftea
*/
#if (__USE_BULLET__)
#include "physics2/bullet/bulletcontroller.h"
namespace Physics2
{
class Controller : public Bullet::BulletController
{
    __DeclareClass(Controller);      
};
}
#elif(__USE_PHYSX__)
#include "physics2/physx/physxcontroller.h"
namespace Physics2
{
class Controller : public PhysX::PhysXController
{
	__DeclareClass(Controller);	 
};
}
#else
#error "Physics2::Controller not implemented"
#endif