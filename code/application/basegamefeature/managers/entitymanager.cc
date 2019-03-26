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
		Util::Array<Util::Delegate<void(Entity)>>& delegates = this->deletionCallbacks[e];
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
uint32_t
EntityManager::GetIndex(const Entity& entity)
{
	return Ids::Index(entity.id);
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
EntityManager::InvalidateAllEntities()
{
	// Create a new pool
	this->pool = Ids::IdGenerationPool();
	this->numEntities = 0;
	this->deletionCallbacks.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
EntityManager::RegisterDeletionCallback(const Entity & e, ComponentInterface* component)
{
	this->RegisterDeletionCallback(e, Util::Delegate<void(Entity)>::FromMethod<ComponentInterface, &ComponentInterface::OnEntityDeleted>(component));
}

//------------------------------------------------------------------------------
/**
	@todo	this can be made available for any callback if we can just
			keep track of callbacks with some listener id or similar.
*/
void
EntityManager::RegisterDeletionCallback(const Entity & e, const Util::Delegate<void(Entity)>& callback)
{
	if (this->deletionCallbacks.Contains(e))
	{
		// Entity already has deletion callbacks registered
		this->deletionCallbacks[e].Append(callback);
		return;
	}

	// Add new entry
	Util::Array<Util::Delegate<void(Entity)>> delArray;
	delArray.Append(callback);
	this->deletionCallbacks.Add(e, delArray);
}

//------------------------------------------------------------------------------
/**
*/
void
EntityManager::DeregisterDeletionCallback(const Entity & e, ComponentInterface* component)
{
	n_assert2(this->deletionCallbacks.Contains(e), "Entity does not have a deletion callback registered for this component!");

	Util::Array<Util::Delegate<void(Entity)>>& delegates = this->deletionCallbacks[e];
	for (SizeT i = 0; i < delegates.Size(); ++i)
	{
		if (delegates[i].GetObject<ComponentInterface>() == component)
		{
			delegates.EraseIndexSwap(i);
			return;
		}
	}
}

} // namespace Game


