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
#include "game/category.h"
#include "memdb/database.h"
#include "basegamefeature/properties/owner.h"

namespace Game
{

/// Create a new entity
Game::Entity CreateEntity(EntityCreateInfo const& info);

/// delete entity
void DeleteEntity(Game::Entity entity);

/// Get instanceid of entity
Game::InstanceId GetInstanceId(Game::Entity entity);

/// Check if an entity ID is still valid.
bool IsValid(Entity e);

/// Check if an entity is active (has an instance). It might be valid, but inactive just after it has been created.
bool IsActive(Entity e);

/// Returns number of active entities
uint GetNumEntities();

/// check if category exists
bool CategoryExists(Util::StringAtom name);

/// return a category id by name
CategoryId const GetCategoryId(Util::StringAtom name);

/// returns the entity mapping of an entity
EntityMapping GetEntityMapping(Game::Entity entity);

/// return an attribute id
PropertyId const GetPropertyId(Util::StringAtom name);

/// Returns the world db
Ptr<MemDb::Database> GetWorldDatabase();

/// Get number of instances in a specific category
SizeT GetNumInstances(CategoryId category);

//------------------------------------------------------------------------------
/**
	@class	Game::EntityManager
	
	Contains state with categories and the world database.

	Generally, you won't need to access any of the methods or
	variables within this manager directly.

	The entity manager handles all entity categories in the game.
	Categories are collections of attributes, arranged as a table where each row
	is an instance, mapped to an entity.
*/
class EntityManager
{
	__DeclareSingleton(EntityManager);
public:
	/// retrieve the api
	static ManagerAPI Create();

	/// destroy entity manager
	static void Destroy();

	/// Adds a category
	CategoryId AddCategory(CategoryCreateInfo const& info);

	/// Returns a category by name. asserts if category does not exist
	Category const& GetCategory(Util::StringAtom name) const;

	/// Returns a category by id. asserts if category does not exist
	Category const& GetCategory(CategoryId cid) const;

	/// Returns the number of existing categories
	SizeT const GetNumCategories() const;

	/// Allocate instance for entity in category instance table
	InstanceId AllocateInstance(Entity entity, CategoryId category);

	/// Deallocated and recycle instance in category instance table
	void DeallocateInstance(Entity entity);

	// Don't modify state without knowing what you're doing!
	struct State
	{
		struct AllocateInstanceCommand
		{
			Game::Entity entity;
			EntityCreateInfo info;
		};

		struct DeallocInstanceCommand
		{
			Game::Entity entity;
		};

		/// Generation pool
		Ids::IdGenerationPool pool;

		/// Number of entities alive
		SizeT numEntities;

		/// Contains the entire world database
		Ptr<MemDb::Database> worldDatabase;

		Util::Queue<AllocateInstanceCommand> allocQueue;
		Util::Queue<DeallocInstanceCommand> deallocQueue;

		// - Categories -
		Util::Array<Category> categoryArray;
		Util::HashTable<Util::StringAtom, CategoryId> catIndexMap;

		Util::Array<EntityMapping> entityMap;

		PropertyId ownerId;
	} state;

private:
	/// constructor
	EntityManager();
	/// destructor
	~EntityManager();
};

//------------------------------------------------------------------------------
/**
*/
inline Category const&
EntityManager::GetCategory(Util::StringAtom name) const
{
	n_assert(this->state.catIndexMap.Contains(name));
	return this->state.categoryArray[this->state.catIndexMap[name].id];
}

//------------------------------------------------------------------------------
/**
*/
inline Category const&
EntityManager::GetCategory(CategoryId cid) const
{
	return this->state.categoryArray[cid.id];
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT const
EntityManager::GetNumCategories() const
{
	return this->state.categoryArray.Size();
}

//-------------------------

} // namespace Game
