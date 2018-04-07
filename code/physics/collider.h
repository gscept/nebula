#pragma once
//------------------------------------------------------------------------------
/**
@class Physics::Collider

    A physics Collider

	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#if (__USE_BULLET__)
#include "physics/bullet/bulletcollider.h"
namespace Physics
{
class Collider : public Bullet::BulletCollider
{
    __DeclareClass(Collider);       
};
}
#elif(__USE_PHYSX__)
#include "physics/physx/physxcollider.h"
namespace Physics
{
class Collider : public PhysX::PhysXCollider
{
	__DeclareClass(Collider);	   
};
}
#elif(__USE_HAVOK__)	
#include "physics/havok/havokcollider.h"
namespace Physics
{
class Collider : public Havok::HavokCollider
{
	__DeclareClass(Collider);	   
};
}
#else
#error "Physics::Collider not implemented"
#endif