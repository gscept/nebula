#pragma once
//------------------------------------------------------------------------------
/**
@class Physics::HingeJoint

    A physics Joint

	(C) 2012-2018 Individual contributors, see AUTHORS file
*/
#if (__USE_BULLET__)
#include "physics/bullet/bullethinge.h"
namespace Physics
{
class HingeJoint : public Bullet::BulletHinge
{
    __DeclareClass(HingeJoint);       
};
}
#elif(__USE_PHYSX__)
#include "physics/physx/physxhinge.h"
namespace Physics
{
class HingeJoint : public PhysX::PhysXHinge
{
	__DeclareClass(HingeJoint);	  
};
}
#elif(__USE_HAVOK__)
#include "physics/havok/havokhinge.h"
namespace Physics
{
class HingeJoint : public Havok::HavokHinge
{
	__DeclareClass(HingeJoint);	  
};
}
#else
#error "Physics::HingeJoint not implemented"
#endif