//------------------------------------------------------------------------------
//  physicsmodel.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "physics/model/physicsmodel.h"
#include "physics/staticobject.h"


using namespace Math;
using namespace Util;

namespace Physics
{

__ImplementClass(Physics::PhysicsModel, 'PHMO', Resources::Resource);


//------------------------------------------------------------------------------
/**
*/
Util::Array<Ptr<PhysicsObject>> 
PhysicsModel::CreateStaticInstance(const Math::matrix44& worldMatrix)
{
	Util::Array<Ptr<PhysicsObject>> ret;
	
	Array<KeyValuePair<Util::String, Ptr<Collider>>>  collidersArray =this->colliders.Content();
	Array<KeyValuePair<Util::String, Ptr<Collider>>>::Iterator piter;
	for(piter=collidersArray.Begin();piter!=collidersArray.End();piter++)
	{
		PhysicsCommon c;							
		c.name = piter->Key();		
		c.collider = piter->Value();
		c.type = StaticObject::RTTI.GetFourCC();
		c.startTransform = worldMatrix;
		Ptr<PhysicsObject> body = PhysicsObject::CreateFromTemplate(c);			
		ret.Append(body);
	}

	return ret;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<PhysicsObject>
PhysicsModel::CreateDynamicInstance(const Math::matrix44& worldMatrix)
{	
	Array<KeyValuePair<Util::String, Ptr<Collider>>>  collidersArray =this->colliders.Content();
	n_assert2(collidersArray.Size() == 1, "only single collider support");

	PhysicsCommon c;							
	c.name = this->name;
	c.collider = collidersArray.Begin()->Value();
	c.type = PhysicsBody::RTTI.GetFourCC();
	c.mass = 1;
	c.bodyFlags = 0;
	c.startTransform = worldMatrix;
	Ptr<PhysicsObject> body = PhysicsObject::CreateFromTemplate(c);			
	return body;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Ptr<Physics::PhysicsObject>> 
PhysicsModel::CreateInstance()
{
	n_assert2(this->objects.Size()>0,"no physics objects in resource");
	Util::Array<Ptr<PhysicsObject>> ret;

	Util::Array<PhysicsCommon>::Iterator iter;
	for(iter = this->objects.Begin();iter!=this->objects.End();iter++)
	{
		Ptr<PhysicsObject> body = PhysicsObject::CreateFromTemplate(*iter);
		ret.Append(body);
	}
	return ret;
}

}
