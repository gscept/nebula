#pragma once
//------------------------------------------------------------------------------
/**
@class Physics::Scene

    A physics Scene

    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#if (__USE_BULLET__)
#include "physics/bullet/bulletscene.h"
namespace Physics
{
class Scene : public Bullet::BulletScene
{
    __DeclareClass(Scene);      
};
}
#elif(__USE_PHYSX__)
#include "physics/physx/physxscene.h"
namespace Physics
{
class Scene : public PhysX::PhysXScene
{
	__DeclareClass(Scene);	  
};
}
#elif(__USE_HAVOK__)	
#include "physics/havok/havokscene.h"
namespace Physics
{
class Scene : public Havok::HavokScene
{
	__DeclareClass(Scene);	  
};
}
#else
#error "Physics::Scene not implemented"
#endif