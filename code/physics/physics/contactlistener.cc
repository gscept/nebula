//------------------------------------------------------------------------------
//  contactlistener.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/contactlistener.h"

namespace Physics
{
#if (__USE_BULLET__)
	__ImplementClass(Physics::ContactListener, 'CTLR', Physics::BaseContactListener);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics::ContactListener, 'CTLR', Physics::BaseContactListener);
#elif(__USE_HAVOK__)
	__ImplementClass(Physics::ContactListener, 'CTLR', Havok::HavokContactListener);
#else
#error "Physics::ContactListener not implemented"
#endif
}


