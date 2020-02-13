//------------------------------------------------------------------------------
//  categorymanager.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "categorymanager.h"
#include "entitymanager.h"

namespace Game
{

__ImplementClass(Game::CategoryManager, 'MCAT', Game::Manager);
__ImplementSingleton(CategoryManager)

//------------------------------------------------------------------------------
/**
*/
CategoryManager::CategoryManager()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
CategoryManager::~CategoryManager()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
CategoryManager::AddCategory(CategoryCreateInfo const& info)
{
	TableCreateInfo tableInfo;
	tableInfo.name = info.name;
	tableInfo.columns.SetSize(info.columns.Size() + 1);

	int i = 0;
	for (auto const& col : info.columns)
		tableInfo.columns[i++] = col;

	// Add the owner attribute to the table.
	tableInfo.columns[i] = Attr::Runtime::OwnerId;


	Ptr<Game::Database> db = EntityManager::Instance()->GetWorldDatabase();

	Category cat;
	cat.name = info.name;
	// Create an instance table
	cat.instanceTable = db->CreateTable(tableInfo);
	// Create a identical table that can hold templates of this category
	cat.templateTable = db->CreateTable(tableInfo);

	this->catIndexMap.Add(cat.name, this->categoryArray.Size());
	this->categoryArray.Append(cat);
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
CategoryManager::AllocateInstance(Entity entity, CategoryId category)
{
	n_assert(EntityManager::Instance()->IsValid(entity));
	n_assert(category < this->categoryArray.Size());
	Category& cat = this->categoryArray[category.id];
	Ptr<Game::Database> db = EntityManager::Instance()->GetWorldDatabase();
	InstanceId instance = db->AllocateRow(cat.instanceTable);
	if (this->entityMap.Size() <= Ids::Index(entity.id))
	{
		this->entityMap.Grow();
		// Just make sure we don't assert on any entity id
		this->entityMap.SetSize(this->entityMap.Capacity());
	}
	this->entityMap[entity.id] = { category, instance };
	return instance;
}



} // namespace Game


