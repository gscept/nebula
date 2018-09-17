#pragma once
//------------------------------------------------------------------------------
/**
    Physics mesh stub

	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#if (__USE_BULLET__)
#include "physics/bullet/bulletphysicsmesh.h"
namespace Physics
{
class PhysicsMesh : public Bullet::BulletPhysicsMesh
{
    __DeclareClass(PhysicsMesh);     
};
}
#elif(__USE_PHYSX__)
#include "physics/physx/physxphysicsmesh.h"
namespace Physics
{
class PhysicsMesh : public PhysX::PhysXPhysicsMesh
{
	__DeclareClass(PhysicsMesh);	 
};
}
#elif(__USE_HAVOK__)
#include "physics/havok/havokphysicsmesh.h"
namespace Physics
{
class PhysicsMesh : public Havok::HavokPhysicsMesh
{
	__DeclareClass(PhysicsMesh);	 
};
}
#else
#error "Physics::PhysicsMesh not implemented"
#endif