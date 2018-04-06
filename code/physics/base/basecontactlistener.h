#pragma once
//------------------------------------------------------------------------------
/**
    @class Physics::BaseContactListener
    
    Base class for a contact listener which is attached to a physics body.
    
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "physics/physicsobject.h"
//------------------------------------------------------------------------------

namespace Physics
{
class PhysicsBody;

class BaseContactListener : public Core::RefCounted
{
	__DeclareClass(Physics::BaseContactListener);
public:

	/// contains a number of contacts that occurs during a collision between two objects
	struct CollisionData
	{
		/// default constructor
		CollisionData():
			object1(NULL),
			object2(NULL)
		{ /* empty */ }

		/// constructor
		CollisionData(Physics::PhysicsObject* obj1, Physics::PhysicsObject* obj2):
			object1(obj1),
			object2(obj2)
		{ /* empty */ }

		/// clear object references
		void Clear();

		Physics::PhysicsObject* object1, *object2;

		Util::Array<float> distances;
		Util::Array<Math::point> points;
		Util::Array<Math::vector> normals;
	};

	/// constructor
	BaseContactListener();
	/// destructor
	virtual ~BaseContactListener();

	/// returns whether there are at least one collision with another body
	bool IsColliding();
	/// get the number of collisions
	SizeT GetNumCollisions();

	/// attach the listener to a physics object
	virtual void AttachToObject(const Ptr<Physics::PhysicsObject>& object);

	/// get the physicsobject this contactlistener is attached to
	Ptr<Physics::PhysicsObject> GetOwner();
	/// returns if the owner is colliding with the object
	bool IsOwnerCollidingWith(const Ptr<Physics::PhysicsObject>& object);
	/// get collision data with object
	CollisionData* GetCollisionDataWithObject(const Ptr<Physics::PhysicsObject>& object) const;
	/// get collision data at index
	CollisionData* GetCollisionDataAtIndex(IndexT i) const;

	/// traverse current collisions and release object references
	virtual void Clear();

protected:

	/// a new collision has started with another physics object
	virtual void OnCollisionAdded(const Ptr<Physics::PhysicsObject>& object);
	/// a collision has ended
	virtual void OnCollisionRemoved(const Ptr<Physics::PhysicsObject>& object);
	/// when there's a new contact during a collision
	void OnNewContactPoint(const Ptr<Physics::PhysicsObject>& object, const Math::point& point, const Math::vector& normal, float distance);

	Ptr<Physics::PhysicsObject> owner;
	Util::Dictionary<Physics::PhysicsObject::Id, CollisionData*> collisions;	//< a map of collision data with other objects
}; 

//------------------------------------------------------------------------------
/**
*/
inline bool 
BaseContactListener::IsColliding()
{
	return this->GetNumCollisions() > 0;
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
BaseContactListener::GetNumCollisions()
{
	return this->collisions.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline Ptr<Physics::PhysicsObject> 
BaseContactListener::GetOwner()
{
	return this->owner;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
BaseContactListener::IsOwnerCollidingWith(const Ptr<Physics::PhysicsObject>& object)
{
	return this->collisions.Contains(object->GetUniqueId());
}

//------------------------------------------------------------------------------
/**
*/
inline BaseContactListener::CollisionData* 
BaseContactListener::GetCollisionDataAtIndex(IndexT i) const
{
	n_assert(this->collisions.Size() > i);

	return this->collisions.ValueAtIndex(i);
}

//------------------------------------------------------------------------------
/**
*/
inline BaseContactListener::CollisionData* 
BaseContactListener::GetCollisionDataWithObject(const Ptr<Physics::PhysicsObject>& object) const
{
	return this->collisions[object->GetUniqueId()];
}

} 
// namespace Physics
//------------------------------------------------------------------------------