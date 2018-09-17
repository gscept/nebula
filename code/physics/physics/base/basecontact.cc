//------------------------------------------------------------------------------
//  basecontact.cc
//  (C) 2005 Radon Labs GmbH
//  (C) 2012 - 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/physicsobject.h"
#include "physics/physicsserver.h"
#include "basecontact.h"

namespace Physics
{
__ImplementClass(Physics::BaseContact, 'BCTP', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
BaseContact::BaseContact() :
	upVector(0.0f, 1.0f, 0.0f),
	depth(0.f),    
	material(InvalidMaterial),
	type(InvalidType)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
BaseContact::~BaseContact()
{	
	this->Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
BaseContact::Clear()
{
	this->normals.Clear();
	this->positions.Clear();
	this->object = 0;
	this->material = InvalidMaterial;
	this->type = InvalidType;
}

//------------------------------------------------------------------------------
/**
    Returns pointer to rigid body of contact point. 
*/
const Ptr<Physics::PhysicsObject> &
BaseContact::GetCollisionObject() const
{   
	return this->object;	
}

//------------------------------------------------------------------------------
/**
*/
void
BaseContact::SetCollisionObject(const Ptr<PhysicsObject> & p)
{
	this->object = p;
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseContact::SetOwnerObject(const Ptr<Physics::PhysicsObject> & b)
{
	this->ownerObject = b;
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<Physics::PhysicsObject>& 
BaseContact::GetOwnerObject() const
{
	return this->ownerObject;
}

}