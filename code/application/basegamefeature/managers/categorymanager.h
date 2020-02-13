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

namespace Game
{

/// describes a category
struct Category
{
	Util::StringAtom name;
	TableId instanceTable;
	TableId templateTable;
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

private:
	Util::Array<Category> categoryArray;
	Util::HashTable<Util::StringAtom, CategoryId> catIndexMap;

	struct EntityMapping
	{
		CategoryId category;
		InstanceId instance;
	};

	Util::Array<EntityMapping> entityMap;
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
