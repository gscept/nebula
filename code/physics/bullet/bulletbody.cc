//------------------------------------------------------------------------------
//  bulletbody.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "btBulletCollisionCommon.h"
#include "btBulletDynamicsCommon.h"
#include "physics/bullet/conversion.h"
#include "physics/physicsbody.h"
#include "physics/bullet/nebulamotionstate.h"

namespace Bullet
{
__ImplementClass(Bullet::BulletBody, 'PBBO', Physics::BaseRigidBody);

using namespace Math;
using namespace Physics;

//------------------------------------------------------------------------------
/**
*/
BulletBody::BulletBody() : 
	body(NULL),
	hasScale(false)
{	
	this->common.type = PhysicsBody::RTTI.GetFourCC();
}

//------------------------------------------------------------------------------
/**
*/
BulletBody::~BulletBody()
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
BulletBody::SetMass(float m)
{	
	this->common.mass = m;
	if(this->body)
	{
		btVector3 inertia(0,0,0);	
		this->common.collider.downcast<BulletCollider>()->GetBulletShape()->calculateLocalInertia(m,inertia);		
		this->body->setMassProps(m,inertia);
	}
}

//------------------------------------------------------------------------------
/**
    Set the body's current linear velocity.

    @param  v   linear velocity
*/
void
BulletBody::SetLinearVelocity(const Math::vector& v)
{
	btVector3 btVel = Neb2BtVector(v);
	this->body->setLinearVelocity(btVel);
	this->body->activate(true);
}

//------------------------------------------------------------------------------
/**
    Get the body's current linear velocity.

    @param  v   [out] linear velocity
*/
Math::vector
BulletBody::GetLinearVelocity() const
{
   return Bt2NebVector(this->body->getLinearVelocity());
}

//------------------------------------------------------------------------------
/**
    Set the body's current angular velocity.

    @param  v   linear velocity
*/
void
BulletBody::SetAngularVelocity(const Math::vector& v)
{    
    btVector3 btVel = Neb2BtVector(v);
	this->body->setAngularVelocity(btVel);
	this->body->activate(true);
}

//------------------------------------------------------------------------------
/**
    Get the body's current angular velocity.

    @param  v   [out] linear velocity
*/
Math::vector
BulletBody::GetAngularVelocity() const
{    
	return Bt2NebVector(this->body->getAngularVelocity());
}

//------------------------------------------------------------------------------
/**
*/
void
BulletBody::SetSleeping(bool enable)
{
	if(!enable) 
		this->body->setActivationState(ACTIVE_TAG);
	else
		this->body->setActivationState(DISABLE_SIMULATION);    
}

//------------------------------------------------------------------------------
/**
*/
bool 
BulletBody::GetSleeping()
{
	n_assert(this->body);
	return !this->body->isActive();
}

//------------------------------------------------------------------------------
/**
*/
void
BulletBody::SetEnabled(bool b)
{
	BaseRigidBody::SetEnabled(b);
	n_assert(this->body);
    if (b) 
		this->body->setActivationState(ACTIVE_TAG);
	else
		this->body->setActivationState(DISABLE_SIMULATION);    
}


//------------------------------------------------------------------------------
/**
	set linear damping factor
*/
void BulletBody::SetLinearDamping(float f){
	body->setLinearDamping(f);
}
//------------------------------------------------------------------------------
/**
	get linear damping factor
*/
float BulletBody::GetLinearDamping() const{
	return body->getLinearDamping();
}


//------------------------------------------------------------------------------
/**
*/
void
BulletBody::SetEnableGravity(bool enable)
{
	n_assert(this->body);
	if(enable)
	{		
		this->body->setGravity(Neb2BtVector(PhysicsServer::Instance()->GetScene()->GetGravity()));
	}else
	{
		this->body->setGravity(btVector3(0.0f,0.0f,0.0f));
	}    
	
}

//------------------------------------------------------------------------------
/**
*/
bool 
BulletBody::GetEnableGravity() const 
{
	n_assert(this->body);
	if(this->body->getGravity().length2() > 0.0f)
		return true;
	else
		return false;
}

//------------------------------------------------------------------------------
/**
    Reset the force and torque accumulators of the body.
*/
void
BulletBody::Reset()
{
	n_assert(this->body);
	this->body->clearForces();    
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletBody::Attach(BaseScene *inWorld)
{

	n_assert(!this->IsAttached());
	n_assert(this->common.collider.isvalid());

	this->world = ((BulletScene *)inWorld)->GetWorld();

	motion = n_new(NebulaMotionState(this));
	
	motion->setWorldTransform(Neb2BtM44Transform(this->transform));

	float4 bodyscale;
	this->transform.get_scale(bodyscale);
	if(bodyscale != float4(1,1,1,1))
	{
		// we have a scaled physics body, need to create a copy of the old collision shape and set a local scaling on it
		Ptr<BulletCollider> coll = this->common.collider.cast<BulletCollider>()->GetScaledCopy(bodyscale);
		this->common.collider = 0;
		this->common.collider = coll.cast<Physics::Collider>();	
		this->scale = bodyscale;
		this->hasScale = true;
	}
	
	btVector3 invInertia(0,0,0);
	this->common.collider.downcast<BulletCollider>()->GetBulletShape()->calculateLocalInertia(this->common.mass, invInertia);

	btRigidBody::btRigidBodyConstructionInfo boxInfo(this->common.mass, motion, this->common.collider.downcast<BulletCollider>()->GetBulletShape(), invInertia);
	this->body = n_new(btRigidBody(boxInfo));
	
	this->ApplyMaterial();
	
	// Add the body to the world. o-o
	this->world->addRigidBody(this->body);	

	this->body->setUserPointer(this);	

	// FIXME
	// fix proper simulation flags (static/dynamic/kinematic)
	const bool isKinematic = (this->common.bodyFlags & Physics::Kinematic) > 0;
	if(isKinematic)
	{
		this->body->setCollisionFlags( this->body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
	}
	this->attached = true;
	this->SetEnabled(true);
	this->common.category = (Physics::CollideCategory)this->body->getBroadphaseProxy()->m_collisionFilterGroup;
	this->common.collideFilterMask = this->body->getBroadphaseProxy()->m_collisionFilterMask;
}

//------------------------------------------------------------------------------
/**
*/
void
BulletBody::Detach()
{
	this->world->removeRigidBody(body);
	n_delete(this->motion);
	n_delete(this->body);
	this->body = 0;
	this->motion = 0;
	this->world = 0;	
	this->attached = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletBody::SetKinematic(bool flag)
{
	if (flag)
	{
		this->body->setCollisionFlags( this->body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);		
	}
	else
	{
		this->body->setCollisionFlags( this->body->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);		
	}	
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletBody::SetTransform(const Math::matrix44& m)
{
	btTransform trans = Neb2BtM44Transform(m);
	this->motion->setWorldTransform(trans);
	this->body->setWorldTransform(trans);
}

//------------------------------------------------------------------------------
/**
*/
const Math::matrix44&
BulletBody::GetTransform()
{	
	if(!this->GetSleeping())
	{
		Math::matrix44 trans = Bt2NebTransform(this->body->getInterpolationWorldTransform());
		if(this->hasScale)
		{
			Math::matrix44 scaletrans = Math::matrix44::scaling(this->scale);
			this->transform = Math::matrix44::multiply(scaletrans,trans);
		}	
		else
		{
			this->transform = trans;
		}
		
	}
	return this->transform;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletBody::OnFrameBefore()
{
	this->transformChanged = false;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletBody::UpdateTransform(const Math::matrix44 & trans)
{

	if(this->hasScale)
	{
		Math::matrix44 scaletrans = Math::matrix44::scaling(this->scale);
		this->transform = Math::matrix44::multiply(scaletrans,trans);
	}	
	else
	{
		this->transform = trans;
	}
	this->transformChanged = true;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletBody::SetCollideCategory(CollideCategory coll)
{
	PhysicsObject::SetCollideCategory(coll);
	this->body->getBroadphaseProxy()->m_collisionFilterGroup = coll;
}

//------------------------------------------------------------------------------
/**
*/
void 
BulletBody::SetCollideFilter(uint mask)
{
	PhysicsObject::SetCollideFilter(mask);
	this->body->getBroadphaseProxy()->m_collisionFilterMask = mask;

}

//------------------------------------------------------------------------------
/**
    Apply an impulse vector at a position in the global coordinate frame.
	FIXME: multiplybymass does nothing atm
*/
void
BulletBody::ApplyImpulseAtPos(const Math::vector& impulse, const Math::point& pos, bool multiplyByMass)
{
    n_assert(this->IsAttached());

	Math::matrix44 m = Math::matrix44::inverse(this->GetTransform());	
	vector rpos = Math::matrix44::transform(pos,m);
	btVector3 bPos = Neb2BtVector(rpos);
	btVector3 bImpulse = Neb2BtVector(impulse);	
	this->body->applyImpulse(bImpulse,bPos);
    this->body->activate(true);	
}

//------------------------------------------------------------------------------
/** 
	Returns friction ratio of the physics body
*/
float BulletBody::GetFriction(){
	return body->getFriction();
}
//------------------------------------------------------------------------------
/** 
	Returns friction ratio of the physics body
*/
void BulletBody::SetFriction(float ratio){
	body->setFriction(ratio);
}


//------------------------------------------------------------------------------
/** 
	Returns restitution ratio of the physics body
*/
float BulletBody::GetRestitution(){
	return body->getRestitution();	
}
//------------------------------------------------------------------------------
/** 
	Sets restitution ratio of the physics body
*/
void BulletBody::SetRestitution(float ratio){
	body->setRestitution(ratio);
}

//------------------------------------------------------------------------------
/** 
	set angular factor for body
*/
void 
BulletBody::SetAngularFactor(const Math::vector& v)
{
	body->setAngularFactor(Neb2BtVector(v));
}

//------------------------------------------------------------------------------
/** 
	set angular factor for body
*/
Math::vector 
BulletBody::GetAngularFactor() const
{
	return Bt2NebVector(body->getAngularFactor());
}

//------------------------------------------------------------------------------
/**
*/
void
BulletBody::SetAngularDamping(float f)
{
	this->body->setDamping(this->body->getLinearDamping(), f);
}

//------------------------------------------------------------------------------
/**
*/
float
BulletBody::GetAngularDamping() const
{
	return this->body->getAngularDamping();
}

//------------------------------------------------------------------------------
/**
*/
void
BulletBody::SetMaterialType(Physics::MaterialType t)
{
	PhysicsObject::SetMaterialType(t);
	if (this->attached && t != Physics::InvalidMaterial)
	{
		float friction = Physics::MaterialTable::GetFriction(t);
		float rest = Physics::MaterialTable::GetRestitution(t);
		this->SetFriction(friction);
		this->SetRestitution(rest);
	}
}

//------------------------------------------------------------------------------
/**
	Applies material properties from the common object. If a valid material 
	is selected, it overrides friction and restitution values in the object
*/
void
BulletBody::ApplyMaterial()
{
	if (this->common.material != Physics::InvalidMaterial)
	{
		this->SetMaterialType(this->common.material);
	}
	else
	{
		if (this->common.friction < 0.0f)
		{
			// Set default values!
			body->setFriction(0.5f); // <-- same as in bullet, but for clarity
		}
		else
		{
			body->setFriction(this->common.friction);
		}

		if (this->common.restitution < 0.0f)
		{
			// Set default values!
			body->setRestitution(0.2f); // <-- same as in bullet, but for clarity
		}
		else
		{
			body->setRestitution(this->common.restitution);
		}
	}	
}

}