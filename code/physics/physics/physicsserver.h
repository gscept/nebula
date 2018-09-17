#pragma once
//------------------------------------------------------------------------------
/**
	@class Physics::PhysicsServer

    A physics PhysicsServer

    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#if (__USE_BULLET__)
#include "physics/bullet/bulletphysicsserver.h"
namespace Physics
{
class PhysicsServer : public Bullet::BulletPhysicsServer
{
    __DeclareClass(PhysicsServer);
    __DeclareInterfaceSingleton(PhysicsServer);
public:
    /// constructor
    PhysicsServer();
    /// destructor
    virtual ~PhysicsServer();    
};
}
#elif(__USE_PHYSX__)
#include "physics/physx/physxphysicsserver.h"
namespace Physics
{
class PhysicsServer : public PhysX::PhysXServer
{
	__DeclareClass(PhysicsServer);
	__DeclareInterfaceSingleton(PhysicsServer);
public:
	/// constructor
	PhysicsServer();
	/// destructor
	virtual ~PhysicsServer();    
};
}
#elif (__USE_HAVOK__)
#include "physics/havok/havokphysicsserver.h"
namespace Physics
{
class PhysicsServer : public Havok::HavokPhysicsServer
{
	__DeclareClass(PhysicsServer);
	__DeclareInterfaceSingleton(PhysicsServer);
public:
	/// constructor
	PhysicsServer();
	/// destructor
	virtual ~PhysicsServer();    
};
}
#else
#error "Physics::PhysicsServer not implemented"
#endif