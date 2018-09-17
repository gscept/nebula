#pragma once
//------------------------------------------------------------------------------
/**
    Physics probe/sensor stub

    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#if (__USE_BULLET__)
#include "physics/bullet/bulletprobe.h"
namespace Physics
{
class PhysicsProbe : public Bullet::BulletProbe
{
    __DeclareClass(PhysicsProbe);
};
}
#elif(__USE_PHYSX__)
#include "physics/physx/physxprobe.h"
namespace Physics
{
class PhysicsProbe : public PhysX::PhysXProbe
{
	__DeclareClass(PhysicsProbe);	 
};
}
#elif(__USE_HAVOK__)
#include "physics/havok/havokprobe.h"
namespace Physics
{
class PhysicsProbe : public Havok::HavokProbe
{
	__DeclareClass(PhysicsProbe);	 
};
}
#else
#error "Physics::PhysicsProbe not implemented"
#endif