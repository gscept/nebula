//------------------------------------------------------------------------------
//  visualdebuggerserver.cc
//  (C) 2013 JM
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/visualdebuggerserver.h"

namespace Physics
{
#if (__USE_BULLET__)
	__ImplementClass(Physics::VisualDebuggerServer, 'VDBS', Physics::BaseVisualDebuggerServer);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics::VisualDebuggerServer, 'VDBS', Physics::BaseVisualDebuggerServer);
#elif(__USE_HAVOK__)	
	__ImplementClass(Physics::VisualDebuggerServer, 'VDBS', Havok::HavokVisualDebuggerServer);
#else
#error "Physics::VisualDebuggerServer not implemented"
#endif
__ImplementInterfaceSingleton(Physics::VisualDebuggerServer);

//------------------------------------------------------------------------------
/**
*/
VisualDebuggerServer::VisualDebuggerServer()
{
	__ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VisualDebuggerServer::~VisualDebuggerServer()
{
	__DestructInterfaceSingleton;
}

}