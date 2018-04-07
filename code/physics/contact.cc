//------------------------------------------------------------------------------
//  contact.cc
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/contact.h"

namespace Physics
{
#if (__USE_BULLET__)
	__ImplementClass(Physics::Contact, 'CTCT', Physics::BaseContact);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics::Contact, 'CTCT', Physics::BaseContact);
#elif(__USE_HAVOK__)
	__ImplementClass(Physics::Contact, 'CTCT', Havok::HavokContact);
#else
#error "Physics::Contact not implemented"
#endif
}


