#pragma once
//------------------------------------------------------------------------------
/**
    Contact listener stub

    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if (__USE_BULLET__)
#include "physics/base/basecontactlistener.h"
namespace Physics
{
class ContactListener : public Physics::BaseContactListener
{
	__DeclareClass(ContactListener);     
};
}
#elif(__USE_PHYSX__)
#include "physics/base/basecontactlistener.h"
namespace Physics
{
class ContactListener : public Physics::BaseContactListener
{
	__DeclareClass(ContactListener);     
};
}
#elif(__USE_HAVOK__)	
#include "physics/havok/havokcontactlistener.h"
namespace Physics
{
class ContactListener : public Havok::HavokContactListener
{
	__DeclareClass(ContactListener);     
};
}
#else
#error "Physics::ContactListener not implemented"
#endif