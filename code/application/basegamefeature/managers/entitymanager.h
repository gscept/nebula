#pragma once
//------------------------------------------------------------------------------
/**
	@class	Game::EntityManager

	Keeps track of all existing entites.

	(C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "ids/idgenerationpool.h"
#include "game/entity.h"
#include "game/manager.h"
#include "util/delegate.h"
#include "game/database/database.h"

namespace Attr
{
__DeclareAttribute(Owner, AccessMode::ReadOnly, Game::Entity, 'OWNR', Game::Entity::Invalid());
}

namespace Game
{

struct FilterSet
{
	/// categories must include all attributes in this array
	Util::Array<Game::AttributeId> inclusive;
	/// categories must NOT contain any attributes in this array
	Util::Array<Game::AttributeId> exclusive;
};

class Dataset
{
public:
	Dataset()
	{
		// empty
	}
	~Dataset()
	{
		// empty
	}

	/// A view into a category table.
	struct CategoryView
	{
		CategoryId cid;
		SizeT numInstances = 0;
		void const* buffers[64];
	};

	Util::Array<CategoryView> const& GetCategories() const
	{
		return this->categories;
	}

	FilterSet const& GetFilterSet() const
	{
		return this->filter;
	}

private:
	friend class EntityManager;
	FilterSet filter;
	/// Category attributes will be in the same order as in the filterset
	Util::Array<CategoryView> categories;
};

class EntityManager : public Game::Manager
{
	__DeclareClass(EntityManager)
	__DeclareSingleton(EntityManager)
public:
	/// constructor
	EntityManager();
	/// destructor
	~EntityManager();

	/// Generate a new entity from a category
	Entity CreateEntity();
	
	/// Create n amount of entities at the same time.
	Util::Array<Entity> CreateEntities(uint n);

	/// Delete an entity.
	void DeleteEntity(const Entity& e);
	
	/// Check if an entity ID is still valid.
	bool IsValid(const Entity& e) const;

	/// Returns number of active entities
	uint GetNumEntities() const;

	/// Returns the world db
	Ptr<Game::Db::Database> GetWorldDatabase() const;

	/// Invalidates all entities and essentially resets the manager.
	void InvalidateAllEntities();

	/// Register a deletion callback to an entity
	void RegisterDeletionCallback(const Entity& e, Util::Delegate<void(Entity)> const& del);
	
	/// Deregister a deletion callback to an entity. Note that this is not super fast.
	void DeregisterDeletionCallback(const Entity& e, Util::Delegate<void(Entity)> const& del);

	/// Query the database for a dataset of tables
	Dataset Query(FilterSet const& filterset);

private:
	friend class CategoryManager;

	/// Generation pool
	Ids::IdGenerationPool pool;

	/// Number of entities alive
	SizeT numEntities;

	/// Contains all callbacks for deletion to components for each entity
	Util::HashTable<Entity, Util::Array<Util::Delegate<void(Entity)>>> deletionCallbacks;

	/// Contains the entire world database
	Ptr<Game::Db::Database> worldDatabase;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
EntityManager::IsValid(const Entity& e) const
{
	return this->pool.IsValid(e.id);
}

//------------------------------------------------------------------------------
/**
*/
inline uint
EntityManager::GetNumEntities() const
{
	return this->numEntities;
}

//------------------------------------------------------------------------------
/**
*/
inline Ptr<Game::Db::Database>
EntityManager::GetWorldDatabase() const
{
	return this->worldDatabase;
}

} // namespace Game
