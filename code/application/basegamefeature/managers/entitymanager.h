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
#include "game/category.h"
#include "game/property.h"

#define SetupAttr(ATTRID) Game::EntityManager::Instance()->AddCategoryAttr(ATTRID)

namespace Attr
{
__DeclareAttribute(Owner, AccessMode::ReadOnly, Game::Entity, 'OWNR', Game::Entity::Invalid());
}

namespace Game
{

/// Create a new entity
Game::Entity CreateEntity(EntityCreateInfo const& info);

void DeleteEntity(Game::Entity entity);

/// Get instanceid of entity
Game::InstanceId GetInstanceId(Game::Entity entity);

/// Check if an entity ID is still valid.
bool IsValid(Entity e);

/// Check if an entity is active (has an instance). It might be valid, but inactive just after it has been created.
bool IsActive(Entity e);

/// Returns number of active entities
uint GetNumEntities();

/// Query the database for a dataset of categories
Dataset Query(FilterSet const& filterset);

/// check if category exists
bool CategoryExists(Util::StringAtom name);

/// return a category id by name
CategoryId const GetCategoryId(Util::StringAtom name);

/// returns the entity mapping of an entity
EntityMapping GetEntityMapping(Game::Entity entity);

/// Returns the world db
Ptr<Game::Db::Database> GetWorldDatabase();

/// Get number of instances in a specific category
SizeT GetNumInstances(CategoryId category);

/// Get an attribute from an entity
template<typename ATTR> typename
ATTR::TYPE const& GetAttribute(Game::Entity entity);

/// Set an attribute of an entity
template<typename ATTR>
void SetAttribute(Game::Entity entity, typename ATTR::TYPE const& value);

/// create a property state and retrieve the buffer
template<typename TYPE> Game::PropertyData<TYPE>
CreatePropertyState(CategoryId category);

///	Retrieve a state buffer from a category
template<typename TYPE> Game::PropertyData<TYPE>
GetPropertyState(CategoryId category);

///	Shortcut for fetching property data buffers(table columns)
template<typename ATTR> Game::PropertyData<typename ATTR::TYPE>
GetPropertyData(CategoryId category);


//------------------------------------------------------------------------------
/**
	@class	Game::EntityManager
	
	Contains state with categories and the world database.

	Generally, you won't need to access any of the methods or
	variables within this manager directly.

	The entity manager handles all entity categories in the game.
	Categories are collections of attributes, arranged as a table where each row
	is an instance, mapped to an entity.

	Categories can have properties attached. When the event methods of this
	manager is called, it subsequently calls the event callbacks for all properties
	for all categories.

	Categories can also have custom, temporary states. These are commonly used
	by properties, and only by the property that uses it. If the data that exists
	in a state needs to be exposed to other properties, you should consider moving
	it to a public attribute.
*/
class EntityManager
{
	__DeclareSingleton(EntityManager);
public:
	/// retrieve the api
	static ManagerAPI Create();

	static void Destroy();

	/// adds a category
	CategoryId AddCategory(CategoryCreateInfo const& info);

	/// returns a category by name. asserts if category does not exist
	Category const& GetCategory(Util::StringAtom name) const;

	/// returns a category by id. asserts if category does not exist
	Category const& GetCategory(CategoryId cid) const;

	/// begin adding category attributes
	void BeginAddCategoryAttrs(Util::StringAtom categoryName);
	/// add a category attribute
	void AddCategoryAttr(const Game::AttributeId& attrId);
	/// add a category property. This automatically adds all attributes for said property
	void AddProperty(const Ptr<Game::Property>& prop);
	/// end adding category attributes
	void EndAddCategoryAttrs();

	/// returns the number of existing categories
	SizeT const GetNumCategories() const;

	/// allocate instance for entity in category instance table
	InstanceId AllocateInstance(Entity entity, CategoryId category);

	/// deallocated and recycle instance in category instance table
	void DeallocateInstance(Entity entity);

	struct AllocateInstanceCommand
	{
		Game::Entity entity;
		EntityCreateInfo info;
	};

	struct DeallocInstanceCommand
	{
		/// index in the entity map
		Game::Entity entity;
	};

	// Don't modify state without knowing what you're doing!
	struct State
	{
		/// Generation pool
		Ids::IdGenerationPool pool;

		/// Number of entities alive
		SizeT numEntities;

		/// Contains the entire world database
		Ptr<Game::Db::Database> worldDatabase;

		Util::Queue<AllocateInstanceCommand> allocQueue;
		Util::Queue<DeallocInstanceCommand> deallocQueue;

		// - Categories -
		Util::Array<Category> categoryArray;
		Util::HashTable<Util::StringAtom, CategoryId> catIndexMap;

		Util::Array<EntityMapping> entityMap;

		// These are used when adding attributes from a property
		bool inBeginAddCategoryAttrs;
		IndexT addAttrCategoryIndex;
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

//------------------------------------------------------------------------------
/**
*/
template<typename ATTR>
typename ATTR::TYPE const&
GetAttribute(Game::Entity entity)
{
	Game::EntityManager const* mgr = EntityManager::Instance();
	auto mapping = GetEntityMapping(entity);
	auto const& cat = mgr->GetCategory(mapping.category);
	void** ptrptr = (*(ATTR::Id().GetCategoryTable()))[mapping.category.id];
	n_assert(ptrptr != nullptr);
	n_assert(*ptrptr != nullptr);

	ATTR::TYPE* data = (ATTR::TYPE*) * ptrptr;
#ifdef NEBULA_BOUNDSCHECKS
	Ptr<Game::Db::Database> db = mgr->state.worldDatabase;
	SizeT const size = db->GetTable(cat.instanceTable).numRows;
	n_assert(mapping.instance.id >= 0 && mapping.instance.id < size);
#endif
	return data[mapping.instance.id];
}

//------------------------------------------------------------------------------
/**
*/
template<typename ATTR>
void
SetAttribute(Game::Entity entity, typename ATTR::TYPE const& value)
{
	n_assert2(ATTR::AccessMode() == Attr::AccessMode::ReadWrite, "Attribute is not directly writable!\n");

	Game::EntityManager const* mgr = EntityManager::Instance();
	auto mapping = GetEntityMapping(entity);
	auto const& cat = mgr->GetCategory(mapping.category);

	if (!ATTR::Id().GetCategoryTable()->Contains(mapping.category.id))
		return;

	auto& ct = *(ATTR::Id().GetCategoryTable());
	void** ptrptr = ct[mapping.category.id];
	n_assert(ptrptr != nullptr);
	n_assert(*ptrptr != nullptr);

	ATTR::TYPE* data = (ATTR::TYPE*) * ptrptr;
#ifdef NEBULA_BOUNDSCHECKS
	Ptr<Game::Db::Database> db = mgr->state.worldDatabase;
	SizeT const size = db->GetTable(cat.instanceTable).numRows;
	n_assert(mapping.instance.id >= 0 && mapping.instance.id < size);
#endif
	data[mapping.instance.id] = value;
}

//------------------------------------------------------------------------------
/**
	Create and retrieve a state buffer from a category
*/
template<typename TYPE>
Game::PropertyData<typename TYPE>
CreatePropertyState(CategoryId category)
{
	Game::EntityManager const* mgr = Game::EntityManager::Instance();
	Ptr<Game::Db::Database> db = mgr->state.worldDatabase;
	Db::TableId const tid = mgr->GetCategory(category).instanceTable;
	return db->AddStateColumn<TYPE>(tid);
}

//------------------------------------------------------------------------------
/**
	Retrieve a state buffer from a category
*/
template<typename TYPE>
Game::PropertyData<typename TYPE>
GetPropertyState(CategoryId category)
{
	Game::EntityManager const* mgr = Game::EntityManager::Instance();
	Ptr<Game::Db::Database> db = mgr->state.worldDatabase;
	Db::TableId const tid = mgr->GetCategory(category).instanceTable;
	n_assert2(db->HasStateColumn(tid, TYPE::ID), "Entity category does not contain state!\n");
	return db->GetStateColumn<TYPE>(tid);
}

//------------------------------------------------------------------------------
/**
	Shortcut for fetching property data buffers(table columns)
*/
template<typename ATTR>
Game::PropertyData<typename ATTR::TYPE>
GetPropertyData(CategoryId category)
{
	Game::EntityManager const* mgr = Game::EntityManager::Instance();
	Ptr<Game::Db::Database> const& db = mgr->state.worldDatabase;
	Db::TableId const tid = mgr->GetCategory(category).instanceTable;
	n_assert2(db->HasColumn(tid, ATTR::Id()), "Category does not have specified attribute!");
	return db->GetColumnData<ATTR>(tid);
}

//-------------------------

} // namespace Game
