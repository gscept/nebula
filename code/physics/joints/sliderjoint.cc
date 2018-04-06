//------------------------------------------------------------------------------
//  sliderjoint.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/joints/sliderjoint.h"

namespace Physics
{
#if (__USE_BULLET__)
	__ImplementClass(Physics::SliderJoint, 'PSLJ', Bullet::BulletSlider);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics::SliderJoint, 'PSLJ', PhysX::PhysXSlider);
#elif(__USE_HAVOK__)
	__ImplementClass(Physics::SliderJoint, 'PSLJ', Havok::HavokSlider);
#else
#error "Physics::SliderJoint not implemented"
#endif
}

