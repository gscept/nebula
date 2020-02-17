#pragma once
//------------------------------------------------------------------------------
/**
	@class	Game::CategoryManager

	(C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "game/entity.h"
#include "game/manager.h"
#include "game/property.h"

#define SetupAttr(ATTRID) Game::CategoryManager::Instance()->AddCategoryAttr(ATTRID)

namespace Game
{

inline SizeT
GetNumInstances(CategoryId category)
{
	Ptr<Game::Database> db = Game::EntityManager::Instance()->GetWorldDatabase();
	return db->GetNumRows(category.id);
}

template<typename TYPE>
Game::PropertyData<typename TYPE> CreatePropertyState(CategoryId category)
{
	Ptr<Game::Database> db = Game::EntityManager::Instance()->GetWorldDatabase();
	return db->AddDataColumn<TYPE>(category.id);
}

/// Shortcut for fetching property data buffers
template<typename ATTR>
Game::PropertyData<typename ATTR::TYPE> GetPropertyData(CategoryId category)
{
	Ptr<Game::Database> db = Game::EntityManager::Instance()->GetWorldDatabase();
	n_assert2(db->HasColumn(category.id, ATTR::Id()), "Category does not have specified attribute!");
	return db->GetColumnData<ATTR>(category.id);
}

/// describes a category
struct Category
{
	Util::StringAtom name;
	TableId instanceTable;
	TableId templateTable;
	Util::Array<Ptr<Game::Property>> properties;
};

typedef Game::TableCreateInfo CategoryCreateInfo;

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
	bool HasCategory(Util::StringAtom name);

	/// adds a category
	void AddCategory(CategoryCreateInfo const& info);
	
	/// returns a category by name. asserts if category does not exist
	Category const& GetCategory(Util::StringAtom name);

	/// return a category id by name
	CategoryId const GetCategoryId(Util::StringAtom name);
	
	//void AddCategoryTemplate

	/// allocate instance for entity in category instance table
	InstanceId AllocateInstance(Entity entity, CategoryId category);

	/// begin adding category attributes
	void BeginAddCategoryAttrs(Util::StringAtom categoryName);
	/// add a category attribute
	void AddCategoryAttr(const Game::AttributeId& attrId);
	/// add a category property. This automatically adds all attributes for said property
	void AddProperty(const Ptr<Game::Property>& prop);
	/// end adding category attributes
	void EndAddCategoryAttrs();

private:
	Util::Array<Category> categoryArray;
	Util::HashTable<Util::StringAtom, CategoryId> catIndexMap;

	struct EntityMapping
	{
		CategoryId category;
		InstanceId instance;
	};

	Util::Array<EntityMapping> entityMap;

	// These are used when adding attributes from a property
	bool inBeginAddCategoryAttrs;
	IndexT addAttrCategoryIndex;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
CategoryManager::HasCategory(Util::StringAtom name)
{
	return this->catIndexMap.Contains(name);
}

//------------------------------------------------------------------------------
/**
*/
inline Category const&
CategoryManager::GetCategory(Util::StringAtom name)
{
	n_assert(this->catIndexMap.Contains(name));
	return this->categoryArray[this->catIndexMap[name].id];
}

//------------------------------------------------------------------------------
/**
*/
inline CategoryId const
CategoryManager::GetCategoryId(Util::StringAtom name)
{
	n_assert(this->catIndexMap.Contains(name));
	return this->catIndexMap[name];
}

} // namespace Game
