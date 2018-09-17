#pragma once
//------------------------------------------------------------------------------
/**
@class Physics::SliderJoint

    A physics Joint

	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#if (__USE_BULLET__)
#include "physics/bullet/bulletslider.h"
namespace Physics
{
class SliderJoint : public Bullet::BulletSlider
{
    __DeclareClass(SliderJoint);       
};
}
#elif(__USE_PHYSX__)
#include "physics/physx/physxslider.h"
namespace Physics
{
class SliderJoint : public PhysX::PhysXSlider
{
	__DeclareClass(SliderJoint);	  
};
}
#elif(__USE_HAVOK__)
#include "physics/havok/havokslider.h"
namespace Physics
{
class SliderJoint : public Havok::HavokSlider
{
	__DeclareClass(SliderJoint);	  
};
}
#else
#error "Physics::SliderJoint not implemented"
#endif