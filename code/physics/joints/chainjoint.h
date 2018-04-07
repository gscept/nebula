#pragma once
//------------------------------------------------------------------------------
/**
@class Physics::ChainJoint

    A physics Joint

	(C) 2013-2016 Individual contributors, see AUTHORS file
*/

#if (__USE_BULLET__)
#include "physics/joints/basechainjoint.h"
namespace Physics
{
class ChainJoint : public Physics::BaseChainJoint
{
    __DeclareClass(ChainJoint);       
};
}
#elif(__USE_PHYSX__)
#include "physics/joints/basechainjoint.h"
namespace Physics
{
class ChainJoint : public Physics::BaseChainJoint
{
	__DeclareClass(ChainJoint);       
};
}
#elif(__USE_HAVOK__)
#include "physics/havok/havokchainjoint.h"
namespace Physics
{
class ChainJoint : public Havok::HavokChainJoint
{
	__DeclareClass(ChainJoint);       
};
}
#else
#error "Physics::ChainJoint not implemented"
#endif