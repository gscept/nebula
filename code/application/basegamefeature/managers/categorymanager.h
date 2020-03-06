#pragma once
//------------------------------------------------------------------------------
/**
	@class	Game::CategoryManager

	The category manager handles all entity categories in the game.
	Categories are collections of attributes, arranged as a table where each row
	is an instance, mapped to an entity.

	Categories can have properties attached. When the event methods of this
	manager is called, it subsequently calls the event callbacks for all properties
	for all categories.
	
	Categories can also have custom, temporary states. These are commonly used
	by properties, and only by the property that uses it. If the data that exists
	in a state needs to be exposed to other properties, yo ushould consider moving
	it to a public attribute.


	@see	factorymanager.h 
	@see	entitymanager.h 

	(C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/entity.h"
#include "game/manager.h"
#include "game/property.h"
#include "entitymanager.h"
#include "debug/debugtimer.h"


#define SetupAttr(ATTRID) Game::CategoryManager::Instance()->AddCategoryAttr(ATTRID)

namespace Game
{

/// get the number of instances of a certain category
SizeT
GetNumInstances(CategoryId category);

/// get instance id of an entity
InstanceId
GetInstanceId(Game::Entity entity);

template<typename ATTR>
typename ATTR::TYPE const& GetAttribute(Game::Entity entity)
{
	Ptr<CategoryManager> mgr = CategoryManager::Instance();
	auto mapping = mgr->GetEntityMapping(entity);
	auto const& cat = mgr->GetCategory(mapping.category);
	void** ptrptr = (*(ATTR::Id().GetCategoryTable()))[mapping.category.id];
	n_assert(ptrptr != nullptr);
	n_assert(*ptrptr != nullptr);

	ATTR::TYPE* data = (ATTR::TYPE*) * ptrptr;
#ifdef NEBULA_BOUNDSCHECKS
	Ptr<Game::Db::Database> db = EntityManager::Instance()->GetWorldDatabase();
	SizeT size = db->GetTable(cat.instanceTable).numRows;
	n_assert(mapping.instance.id >= 0 && mapping.instance.id < size);
#endif
	return data[mapping.instance.id];
}

template<typename ATTR>
void SetAttribute(Game::Entity entity, typename ATTR::TYPE const& value)
{
	n_assert2(ATTR::GetAccessMode() == AccessMode::ReadWrite, "Attribute is not directly writable!\n");

	Ptr<CategoryManager> mgr = CategoryManager::Instance();
	auto mapping = mgr->GetEntityMapping(entity);
	auto const& cat = mgr->GetCategory(mapping.category);
	
	if (!ATTR::Id().GetCategoryTable()->Contains(mapping.category.id))
		return;

	auto& ct = *(ATTR::Id().GetCategoryTable());
	void** ptrptr = ct[mapping.category.id];
	n_assert(ptrptr != nullptr);
	n_assert(*ptrptr != nullptr);

	ATTR::TYPE* data = (ATTR::TYPE*)*ptrptr;
#ifdef NEBULA_BOUNDSCHECKS
	Ptr<Game::Db::Database> db = EntityManager::Instance()->GetWorldDatabase();
	SizeT size = db->GetTable(cat.instanceTable).numRows;
	n_assert(mapping.instance.id >= 0 && mapping.instance.id < size);
#endif
	data[mapping.instance.id] = value;
}

/// create and retrieve a state buffer from a category
template<typename TYPE>
Game::PropertyData<typename TYPE> CreatePropertyState(CategoryId category)
{
	Ptr<Game::Db::Database> db = Game::EntityManager::Instance()->GetWorldDatabase();
	Db::TableId tid = CategoryManager::Instance()->GetCategory(category).instanceTable;
	return db->AddStateColumn<TYPE>(tid);
}

/// retrieve a state buffer from a category
template<typename TYPE>
Game::PropertyData<typename TYPE> GetPropertyState(CategoryId category)
{
	Ptr<Game::Db::Database> db = Game::EntityManager::Instance()->GetWorldDatabase();
	Db::TableId tid = CategoryManager::Instance()->GetCategory(category).instanceTable;
	n_assert2(db->HasStateColumn(tid, TYPE::ID), "Entity category does not contain state!\n");
	return db->GetStateColumn<TYPE>(tid);
}

/// shortcut for fetching property data buffers (table columns)
template<typename ATTR>
Game::PropertyData<typename ATTR::TYPE> GetPropertyData(CategoryId category)
{
	Ptr<Game::Db::Database> db = Game::EntityManager::Instance()->GetWorldDatabase();
	Db::TableId tid = CategoryManager::Instance()->GetCategory(category).instanceTable;
	n_assert2(db->HasColumn(tid, ATTR::Id()), "Category does not have specified attribute!");
	return db->GetColumnData<ATTR>(tid);
}

/// describes a category
struct Category
{
	Util::StringAtom name;
	Db::TableId instanceTable;
	Db::TableId templateTable;
	Util::Array<Ptr<Game::Property>> properties;
};

typedef Game::Db::TableCreateInfo CategoryCreateInfo;

class CategoryManager : public Game::Manager
{
	__DeclareClass(CategoryManager)
	__DeclareSingleton(CategoryManager)
public:
	/// constructor
	CategoryManager();
	/// destructor
	~CategoryManager();

	/// called before frame by the game server
	void OnBeginFrame();
	/// called per-frame by the game server
	void OnFrame();
	/// called after frame by the game server
	void OnEndFrame();

	/// check if category exists
	bool HasCategory(Util::StringAtom name) const;

	/// adds a category
	void AddCategory(CategoryCreateInfo const& info);
	
	/// returns a category by name. asserts if category does not exist
	Category const& GetCategory(Util::StringAtom name) const;
	
	/// returns a category by id. asserts if category does not exist
	Category const& GetCategory(CategoryId cid) const;

	/// return a category id by name
	CategoryId const GetCategoryId(Util::StringAtom name) const;

	/// returns the number of existing categories
	SizeT const GetNumCategories() const;

	// TODO: We need to be able so instantiate templates of categories.
	// 		 Maybe this should be implemented in the factory manager?	
	//void AddCategoryTemplate

	/// allocate instance for entity in category instance table
	InstanceId AllocateInstance(Entity entity, CategoryId category);
	
	/// deallocated and recycle instance in category instance table
	void DeallocateInstance(Entity entity);

	/// begin adding category attributes
	void BeginAddCategoryAttrs(Util::StringAtom categoryName);
	/// add a category attribute
	void AddCategoryAttr(const Game::AttributeId& attrId);
	/// add a category property. This automatically adds all attributes for said property
	void AddProperty(const Ptr<Game::Property>& prop);
	/// end adding category attributes
	void EndAddCategoryAttrs();

	struct EntityMapping
	{
		CategoryId category;
		InstanceId instance;
	};

	EntityMapping GetEntityMapping(Game::Entity entity) const;

private:
	Util::Array<Category> categoryArray;
	Util::HashTable<Util::StringAtom, CategoryId> catIndexMap;

	Util::Array<EntityMapping> entityMap;

	// These are used when adding attributes from a property
	bool inBeginAddCategoryAttrs;
	IndexT addAttrCategoryIndex;

	_declare_timer(CategoryManagerOnBeginFrame)
};

//------------------------------------------------------------------------------
/**
*/
inline bool
CategoryManager::HasCategory(Util::StringAtom name) const
{
	return this->catIndexMap.Contains(name);
}

//------------------------------------------------------------------------------
/**
*/
inline Category const&
CategoryManager::GetCategory(Util::StringAtom name) const
{
	n_assert(this->catIndexMap.Contains(name));
	return this->categoryArray[this->catIndexMap[name].id];
}

//------------------------------------------------------------------------------
/**
*/
inline Category const&
CategoryManager::GetCategory(CategoryId cid) const
{
	return this->categoryArray[cid.id];
}

//------------------------------------------------------------------------------
/**
*/
inline CategoryId const
CategoryManager::GetCategoryId(Util::StringAtom name) const
{
	n_assert(this->catIndexMap.Contains(name));
	return this->catIndexMap[name];
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT const
CategoryManager::GetNumCategories() const
{
	return this->categoryArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline CategoryManager::EntityMapping
CategoryManager::GetEntityMapping(Game::Entity entity) const
{
	return this->entityMap[Ids::Index(entity.id)];
}

} // namespace Game
