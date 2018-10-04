//------------------------------------------------------------------------------
//  basecontactlistener.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "basecontactlistener.h"
#include "../physicsbody.h"

namespace Physics
{
__ImplementClass(Physics::BaseContactListener, 'BCTL', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
BaseContactListener::BaseContactListener()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
BaseContactListener::~BaseContactListener()
{
	this->owner = 0;
	this->collisions.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseContactListener::AttachToObject(const Ptr<Physics::PhysicsObject>& object)
{
	n_assert(object.isvalid());
	this->owner = object;
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseContactListener::OnCollisionAdded(const Ptr<Physics::PhysicsObject>& object)
{
	const PhysicsObject::Id& id = object->GetUniqueId();

	n_assert(!this->collisions.Contains(id));

	this->collisions.Add(id, n_new(CollisionData(this->owner, object)));
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseContactListener::OnCollisionRemoved(const Ptr<Physics::PhysicsObject>& object)
{
	const PhysicsObject::Id& id = object->GetUniqueId();
	n_assert(this->collisions.Contains(id));

	n_delete(this->collisions[id]);
	this->collisions.Erase(id);
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseContactListener::OnNewContactPoint(const Ptr<Physics::PhysicsObject>& object, const Math::point& point, const Math::vector& normal, float distance)
{
	const PhysicsObject::Id& id = object->GetUniqueId();
	
	if (!this->collisions.Contains(id))
	{
		n_printf(" ----- BaseContactListener::OnNewContactPoint: Error, id %d @ %d\n", (int)object->GetUniqueId(), (int)object.get());
		return;
	}

	n_assert(this->collisions.Contains(id));

	CollisionData* collision = this->collisions[id];

	collision->normals.Append(normal);
	collision->points.Append(point);
	collision->distances.Append(distance);
}

//------------------------------------------------------------------------------
/**
*/
void 
BaseContactListener::Clear()
{
	IndexT i;
	for (i = 0; i < this->collisions.Size(); i++)
	{
		CollisionData* collisionData = this->collisions.ValueAtIndex(i);
		collisionData->Clear();
		n_delete(collisionData);
	}

	this->collisions.Clear();
	this->owner = 0;
}


//------------------------------------------------------------------------------
/**
*/
void 
BaseContactListener::CollisionData::Clear()
{
	this->object1 = 0;
	this->object2 = 0;
}

}