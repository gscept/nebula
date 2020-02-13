//------------------------------------------------------------------------------
//  entitymanager.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "entitymanager.h"
#include "game/entity.h"

namespace Attr
{
__DefineAttribute(Owner, Game::Entity, 'OWNR', Game::Entity::Invalid());
}

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
	this->worldDatabase = Game::Database::Create();
}

//------------------------------------------------------------------------------
/**
*/
EntityManager::~EntityManager()
{
	this->worldDatabase = nullptr;
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
Entity
EntityManager::CreateEntity()
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
		arr.Append(this->CreateEntity());
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
	@todo	this can be made available for any callback if we can just
			keep track of callbacks with some listener id or similar.
*/
void
EntityManager::RegisterDeletionCallback(const Entity & e, Util::Delegate<void(Entity)> const& callback)
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
EntityManager::DeregisterDeletionCallback(const Entity & e, Util::Delegate<void(Entity)> const& del)
{
	n_error("NOT IMPLEMENTED!");

	n_assert2(this->deletionCallbacks.Contains(e), "Entity does not have a deletion callback registered for this component!");
	
	Util::Array<Util::Delegate<void(Entity)>>& delegates = this->deletionCallbacks[e];
	for (SizeT i = 0; i < delegates.Size(); ++i)
	{
		//if (delegates[i] == del)
		//{
		//	delegates.EraseIndexSwap(i);
		//	return;
		//}
	}
}

} // namespace Game


