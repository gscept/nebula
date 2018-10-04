#pragma once
//------------------------------------------------------------------------------
/**
    Contact stub

	(C) 2013-2018 Individual contributors, see AUTHORS file
*/
#if (__USE_BULLET__)
#include "physics/base/basecontact.h"
namespace Physics
{
class Contact : public Physics::BaseContact
{
	__DeclareClass(Contact);     
};
}
#elif(__USE_PHYSX__)
#include "physics/base/basecontact.h"
namespace Physics
{
class Contact : public Physics::BaseContact
{
	__DeclareClass(Contact);     
};
}
#elif(__USE_HAVOK__)	
#include "physics/havok/havokcontact.h"
namespace Physics
{
class Contact : public Havok::HavokContact
{
	__DeclareClass(Contact);     
};
}
#else
#error "Physics::Contact not implemented"
#endif