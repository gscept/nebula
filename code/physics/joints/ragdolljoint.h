#pragma once
//------------------------------------------------------------------------------
/**
@class Physics::RagdollJoint

    A physics Joint

    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if (__USE_BULLET__)
#include "physics/joints/baseragdolljoint.h"
namespace Physics
{
class RagdollJoint : public Physics::BaseRagdollJoint
{
    __DeclareClass(RagdollJoint);       
};
}
#elif(__USE_PHYSX__)
#include "physics/joints/baseragdolljoint.h"
namespace Physics
{
class RagdollJoint : public Physics::BaseRagdollJoint
{
	__DeclareClass(RagdollJoint);       
};
}
#elif(__USE_HAVOK__)
#include "physics/havok/havokragdolljoint.h"
namespace Physics
{
class RagdollJoint : public Havok::HavokRagdollJoint
{
	__DeclareClass(RagdollJoint);       
};
}
#else
#error "Physics::RagdollJoint not implemented"
#endif