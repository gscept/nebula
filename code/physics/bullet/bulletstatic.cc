//------------------------------------------------------------------------------
//  bulletstatic.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/bullet/bulletstatic.h"
#include "physics/staticobject.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "physics/bullet/conversion.h"
#include "physics/bullet/bulletscene.h"



namespace Bullet
{
	__ImplementClass(Bullet::BulletStatic, 'PHBS', Physics::BaseStatic);

//------------------------------------------------------------------------------
/**
*/
BulletStatic::BulletStatic():collObj(NULL)
{	
	this->common.type = Physics::StaticObject::RTTI.GetFourCC();
}

//------------------------------------------------------------------------------
/**
*/
BulletStatic::~BulletStatic()
{	
	if(this->attached)
	{
		this->Detach();
	}
	this->common.collider = 0;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletStatic::Attach(Physics::BaseScene * inWorld)
{
	n_assert2(this->common.collider.isvalid(), "No collider object attached");

	world = ((BulletScene *)inWorld)->GetWorld();

	this->collObj = (btCollisionObject *)n_new(btCollisionObject());	

	this->collObj->setCollisionShape(this->common.collider.cast<BulletCollider>()->GetBulletShape());
	this->collObj->setWorldTransform(Neb2BtM44Transform(this->transform));	
	this->collObj->setUserPointer(this);
	world->addCollisionObject(this->collObj,short(btBroadphaseProxy::StaticFilter),short(btBroadphaseProxy::AllFilter ^ btBroadphaseProxy::StaticFilter));
	this->common.category = Physics::Static;
	this->common.collideFilterMask = Physics::All^Physics::Static;
	this->SetMaterialType(this->common.material);
	this->attached = true;
}

//------------------------------------------------------------------------------
/**
*/
void
BulletStatic::Detach()
{
	n_assert2(this->collObj,"detaching empty object");
	world->removeCollisionObject(this->collObj);
	n_delete(this->collObj);
	this->collObj = 0;
	this->attached = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletStatic::SetCollideCategory(Physics::CollideCategory coll)
{
	PhysicsObject::SetCollideCategory(coll);
	this->collObj->getBroadphaseHandle()->m_collisionFilterGroup = coll;
}
//------------------------------------------------------------------------------
/**
*/

void 
BulletStatic::SetCollideFilter(uint mask)
{
	PhysicsObject::SetCollideFilter(mask);
	this->collObj->getBroadphaseHandle()->m_collisionFilterMask = mask;

}

//------------------------------------------------------------------------------
/**
*/
void
BulletStatic::SetTransform(const Math::matrix44 & itrans)
{
	btTransform trans = Neb2BtM44Transform(itrans);	
	this->collObj->setWorldTransform(trans);
	this->transform = itrans;
}

//------------------------------------------------------------------------------
/**
*/
void
BulletStatic::SetMaterialType(Physics::MaterialType t)
{
	PhysicsObject::SetMaterialType(t);
	if (this->attached && t != Physics::InvalidMaterial)
	{
		float friction = Physics::MaterialTable::GetFriction(t);
		float rest = Physics::MaterialTable::GetRestitution(t);
		this->collObj->setFriction(friction);
		this->collObj->setRestitution(rest);
	}	
}

} // namespace Physics