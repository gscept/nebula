#pragma once
//------------------------------------------------------------------------------
/**
@class Physics::UniversalJoint

    A physics Joint

	(C) 2012-2018 Individual contributors, see AUTHORS file
*/
#if (__USE_BULLET__)
#include "physics/bullet/bulletuniversaljoint.h"
namespace Physics
{
class UniversalJoint : public Bullet::BulletUniversalJoint
{
    __DeclareClass(UniversalJoint);       
};
}
#elif(__USE_PHYSX__)
#include "physics/physx/physxuniversaljoint.h"
namespace Physics
{
class UniversalJoint : public PhysX::PhysXUniversalJoint
{
	__DeclareClass(UniversalJoint);	  
};
}
#elif(__USE_HAVOK__)
#include "physics/havok/havokuniversaljoint.h"
namespace Physics
{
class UniversalJoint : public Havok::HavokUniversalJoint
{
	__DeclareClass(UniversalJoint);	  
};
}
#else
#error "Physics::UniversalJoint not implemented"
#endif