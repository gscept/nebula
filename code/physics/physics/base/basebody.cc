//------------------------------------------------------------------------------
//  basebody.cc
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/base/basebody.h"
#include "physics/collider.h"

namespace Physics
{
	__ImplementAbstractClass(Physics::BaseRigidBody, 'PBRB', Physics::PhysicsObject);


//------------------------------------------------------------------------------
/**
*/
BaseRigidBody::BaseRigidBody()
{
	this->common.bodyFlags = 0;
}

//------------------------------------------------------------------------------
/**
*/
BaseRigidBody::~BaseRigidBody()
{

}

//------------------------------------------------------------------------------
/**
*/
void 
BaseRigidBody::SetupFromTemplate(const PhysicsCommon & templ)
{
	PhysicsObject::SetupFromTemplate(templ);	
	this->transform = templ.startTransform;	
}

//------------------------------------------------------------------------------
/**
*/
void
BaseRigidBody::RenderDebug()
{
	PhysicsObject::RenderDebug();
	n_assert2(this->common.collider.isvalid(),"empty collider");
	this->common.collider->RenderDebug(this->GetTransform());
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseRigidBody::SetLinearVelocity(const Math::vector& v)
{
	n_error("BaseRigidBody::SetLinearVelocity: Not implemented");
}

//------------------------------------------------------------------------------
/**
*/
Math::vector 
BaseRigidBody::GetLinearVelocity() const
{
	n_error("BaseRigidBody::GetLinearVelocity: Not implemented");
	Math::vector dummy;
	return dummy;
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseRigidBody::SetAngularVelocity(const Math::vector& v)
{
	n_error("BaseRigidBody::SetAngularVelocity: Not implemented");
}

//------------------------------------------------------------------------------
/**
*/
Math::vector 
BaseRigidBody::GetAngularVelocity() const
{
	n_error("BaseRigidBody::GetAngularVelocity: Not implemented");
	Math::vector dummy;
	return dummy;
}

//------------------------------------------------------------------------------
/**
*/
Math::vector 
BaseRigidBody::GetCenterOfMassLocal()
{
	n_error("BaseRigidBody::GetCenterOfMassLocal: Not implemented");
	return -1;
}

//------------------------------------------------------------------------------
/**
*/
Math::vector 
BaseRigidBody::GetCenterOfMassWorld()
{
	n_error("BaseRigidBody::GetCenterOfMassWorld: Not implemented");
	return -1;
}

} // namespace Physics