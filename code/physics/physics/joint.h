#pragma once
//------------------------------------------------------------------------------
/**
@class Physics::Joint

    A physics Joint

	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#if (__USE_BULLET__)
#include "physics/bullet/bulletjoint.h"
namespace Physics
{
class Joint : public Bullet::BulletJoint
{
    __DeclareClass(Joint);       
};
}
#elif(__USE_PHYSX__)
#include "physics/physx/physxjoint.h"
namespace Physics
{
class Joint : public PhysX::PhysXJoint
{
	__DeclareClass(Joint);	  
};
}
#elif(__USE_HAVOK__)	
#include "physics/havok/havokjoint.h"
namespace Physics
{
class Joint : public Havok::HavokJoint
{
	__DeclareClass(Joint);	  
};
}
#else
#error "Physics::Joint not implemented"
#endif