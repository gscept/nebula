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

/// returns the entity mapping of an entity
EntityMapping GetEntityMapping(Game::Entity entity);

/// return an attribute id
PropertyId const GetPropertyId(Util::StringAtom name);

/// add a property to an entity
void AddProperty(Game::Entity const entity, PropertyId const pid);

/// check if entity has a specific property. (SLOW!)
bool HasProperty(Game::Entity const entity, PropertyId const pid);

/// Set a property
template<typename TYPE>
void SetProperty(Game::Entity const entity, PropertyId const pid, TYPE value);

/// returns a blueprint id
BlueprintId const GetBlueprintId(Util::StringAtom name);

/// returns a template id by name
TemplateId const GetTemplateId(Util::StringAtom name);

/// query the world database for instances with filter
Dataset Query(FilterSet const& filter);

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

	/// creates a category
	CategoryId CreateCategory(CategoryCreateInfo const& info);

	/// returns a category by id. asserts if category does not exist
	Category const& GetCategory(CategoryId cid) const;

	/// returns the number of existing categories
	SizeT const GetNumCategories() const;

	/// allocate instance for entity in category instance table
	InstanceId AllocateInstance(Entity entity, CategoryId category);
	
	/// allocate instance for entity in blueprints category instance table
	InstanceId AllocateInstance(Entity entity, BlueprintId blueprint);

	/// allocate instance for entity in blueprint instance table by copying template
	InstanceId AllocateInstance(Entity entity, TemplateId templateId);

	/// deallocated and recycle instance in category instance table
	void DeallocateInstance(Entity entity);

	/// migrate an instance from one category to another
	InstanceId Migrate(Entity entity, CategoryId newCategory);

	// Don't modify state without knowing what you're doing!
	struct State
	{
		struct AllocateInstanceCommand
		{
			Game::Entity entity;
			TemplateId tid;
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
		Util::HashTable<CategoryHash, CategoryId> catIndexMap;

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

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
void
SetProperty(Game::Entity const entity, PropertyId const pid, TYPE value)
{
	EntityMapping mapping = GetEntityMapping(entity);
	Category const& cat = EntityManager::Singleton->GetCategory(mapping.category);
	Ptr<MemDb::Database> db = EntityManager::Singleton->state.worldDatabase;
	auto cid = db->GetColumnId(cat.instanceTable, pid);
	TYPE* ptr = (TYPE*)db->GetValuePointer(cat.instanceTable, cid, mapping.instance.id);
	*ptr = value;
}

//-------------------------
} // namespace Game
