//------------------------------------------------------------------------------
//  PhysicsServer.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physicsserver.h"

namespace Physics
{
#if (__USE_BULLET__)
	__ImplementClass(Physics::PhysicsServer, 'PHCT', Bullet::BulletPhysicsServer);
#elif(__USE_PHYSX__)
	__ImplementClass(Physics::PhysicsServer, 'PHCT', PhysX::PhysXServer);
#elif (__USE_HAVOK__)
	__ImplementClass(Physics::PhysicsServer, 'PHCT', Havok::HavokPhysicsServer);
#else
#error "Physics::PhysicsServer not implemented"
#endif
__ImplementInterfaceSingleton(Physics::PhysicsServer);

//------------------------------------------------------------------------------
/**
*/
PhysicsServer::PhysicsServer()
{
	__ConstructInterfaceSingleton;
}

//------------------------------------------------------------------------------
/**
*/
PhysicsServer::~PhysicsServer()
{
	__DestructInterfaceSingleton;
}

}
