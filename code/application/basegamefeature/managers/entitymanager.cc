//------------------------------------------------------------------------------
//  entitymanager.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "entitymanager.h"

namespace Game
{

__ImplementClass(Game::EntityManager, 'EnMr', Game::Manager);
__ImplementSingleton(EntityManager)

//------------------------------------------------------------------------------
/**
*/
EntityManager::EntityManager() :
	numEntities(0)
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
EntityManager::~EntityManager()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
Entity
EntityManager::NewEntity()
{
	Entity e;
	this->pool.Allocate(e.id);
	this->numEntities++;
	return e;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Entity>
EntityManager::CreateEntities(uint n)
{
	Util::Array<Entity> arr;
	for (SizeT i = 0; i < n; i++)
	{
		arr.Append(this->NewEntity());
	}
	return arr;
}

//------------------------------------------------------------------------------
/**
*/
void
EntityManager::DeleteEntity(const Entity & e)
{
	this->pool.Deallocate(e.id);

	// Call deletion callbacks for this entity
	if (this->deletionCallbacks.Contains(e))
	{
		Util::Array<Util::Delegate<Entity>>& delegates = this->deletionCallbacks[e];
		for (SizeT i = 0; i < delegates.Size(); ++i)
		{
			n_assert2(delegates[i].IsValid(), "Deletion callback is not valid!");
			delegates[i](e);
		}
		this->deletionCallbacks.Erase(e);
	}

	this->numEntities--;
}

//------------------------------------------------------------------------------
/**
*/
bool
EntityManager::IsAlive(const Entity & e) const
{
	return this->pool.IsValid(e.id);
}

//------------------------------------------------------------------------------
/**
*/
uint
EntityManager::GetNumEntities() const
{
	return this->numEntities;
}

//------------------------------------------------------------------------------
/**
*/
void
EntityManager::RegisterDeletionCallback(const Entity & e, const Ptr<BaseComponent>& component)
{
	Util::Delegate<Entity> d = Util::Delegate<Entity>::FromMethod<BaseComponent, &BaseComponent::OnEntityDeleted>(component);
	
	if (this->deletionCallbacks.Contains(e))
	{
		// Entity already has deletion callbacks registered
		this->deletionCallbacks[e].Append(d);
		return;
	}

	// Add new entry
	Util::Array<Util::Delegate<Entity>> delArray;
	delArray.Append(d);
	this->deletionCallbacks.Add(e, delArray);
}

//------------------------------------------------------------------------------
/**
*/
void
EntityManager::DeregisterDeletionCallback(const Entity & e, const Ptr<BaseComponent>& component)
{
	n_assert2(this->deletionCallbacks.Contains(e), "Entity does not have a deletion callback registered for this component!");

	Util::Array<Util::Delegate<Entity>>& delegates = this->deletionCallbacks[e];
	for (SizeT i = 0; i < delegates.Size(); ++i)
	{
		if (delegates[i].GetObject<Ptr<BaseComponent>>()->HashCode() == component.HashCode())
		{
			delegates.EraseIndexSwap(i);
			return;
		}
	}
}

} // namespace Game


