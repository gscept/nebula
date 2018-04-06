#pragma once
//------------------------------------------------------------------------------
/**
	@class Bullet::BulletPhysicsServer

	(C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "physics/base/physicsserverbase.h"

namespace Bullet
{

class BulletPhysicsServer : public Physics::BasePhysicsServer
{
	__DeclareClass(BulletPhysicsServer);
public:
	BulletPhysicsServer(){}
	~BulletPhysicsServer(){}

	void HandleCollisions();
protected:	
	
private:
};

}