//------------------------------------------------------------------------------
//  bulletprobe.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/bullet/bulletprobe.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "BulletCollision/CollisionDispatch/btGhostObject.h"
#include "physics/bullet/conversion.h"
#include "physics/bullet/bulletscene.h"
#include "physics/physicsobject.h"
#include "physics/physicsprobe.h"
#include <typeinfo>

using namespace Physics;
using namespace Util;

namespace Bullet
{
__ImplementClass(Bullet::BulletProbe, 'PBRO', Physics::BaseStatic);

//------------------------------------------------------------------------------
/**
*/
BulletProbe::BulletProbe() : ghost(NULL)    
{	
	this->common.type = Physics::PhysicsProbe::RTTI.GetFourCC();
}

//------------------------------------------------------------------------------
/**
*/
BulletProbe::~BulletProbe()
{
	if(this->attached)
	{
		Detach();
	}
	this->common.collider = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletProbe::Attach(Physics::BaseScene * inWorld)
{
	n_assert2(this->common.collider.isvalid(), "No collider object attached");

	world = ((BulletScene *)inWorld)->GetWorld();

	this->ghost = (btPairCachingGhostObject *)n_new(btPairCachingGhostObject);
	this->ghost->setUserPointer(this);

	this->transform = this->common.startTransform;

	this->ghost->setCollisionShape(this->common.collider.cast<BulletCollider>()->GetBulletShape());
	this->ghost->setWorldTransform(Neb2BtM44Transform(this->transform));	

	world->addCollisionObject(this->ghost,btBroadphaseProxy::SensorTrigger,btBroadphaseProxy::AllFilter & ~btBroadphaseProxy::SensorTrigger);
	this->ghost->setCollisionFlags(this->ghost->getCollisionFlags()| btCollisionObject::CF_NO_CONTACT_RESPONSE);		
	this->attached = true;
	this->common.category = (Physics::CollideCategory)this->ghost->getBroadphaseHandle()->m_collisionFilterGroup;
	this->common.collideFilterMask = this->ghost->getBroadphaseHandle()->m_collisionFilterMask;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Ptr<Core::RefCounted>> 
BulletProbe::GetOverlappingObjects()
{	

	Util::Array<Ptr<Core::RefCounted>> ret;

	int objs = ghost->getNumOverlappingObjects();

	for(int i=0;i<objs;i++)
	{
		btCollisionObject * collObj = ghost->getOverlappingObject(i);
		if((typeid(*collObj) == typeid(btRigidBody)) || (typeid(*collObj) == typeid(btPairCachingGhostObject)))
		{
			PhysicsObject * user = (PhysicsObject*) collObj->getUserPointer();			
			if(user)
			{
				ret.Append(user->GetUserData()->object);
			}
		}
	}   
	return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
BulletProbe::Detach()
{
	world->removeCollisionObject(this->ghost);
	n_delete(this->ghost);
	this->attached = false;
	this->ghost = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletProbe::SetCollideCategory(Physics::CollideCategory coll)
{
	PhysicsObject::SetCollideCategory(coll);
	this->ghost->getBroadphaseHandle()->m_collisionFilterGroup = coll;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletProbe::SetCollideFilter(uint mask)
{
	PhysicsObject::SetCollideFilter(mask);
	this->ghost->getBroadphaseHandle()->m_collisionFilterMask = mask;

}

//------------------------------------------------------------------------------
/**
*/
void
BulletProbe::SetTransform(const Math::matrix44 & itrans)
{
	btTransform trans = Neb2BtM44Transform(itrans);	
	this->ghost->setWorldTransform(trans);
	this->transform = itrans;
}

}
