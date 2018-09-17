#pragma once
//------------------------------------------------------------------------------
/**
@class Physics::PointToPointjoint

    A physics Joint

	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#if (__USE_BULLET__)
#include "physics/bullet/bulletpointtopoint.h"
namespace Physics
{
class PointToPointjoint : public Bullet::BulletPointToPoint
{
    __DeclareClass(PointToPointjoint);       
};
}
#elif(__USE_PHYSX__)
#include "physics/physx/physxpointtopoint.h"
namespace Physics
{
class PointToPointjoint : public PhysX::PhysXPointToPoint
{
	__DeclareClass(PointToPointjoint);	  
};
}
#elif(__USE_HAVOK__)
#include "physics/havok/havokpointtopoint.h"
namespace Physics
{
class PointToPointjoint : public Havok::HavokPointToPoint
{
	__DeclareClass(PointToPointjoint);	  
};
}
#else
#error "Physics::PointToPointjoint not implemented"
#endif