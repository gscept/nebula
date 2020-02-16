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
CategoryManager::CategoryManager() :
    inBeginAddCategoryAttrs(false),
    addAttrCategoryIndex(0)
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
CategoryManager::OnBeginFrame()
{
	for (IndexT i = 0; i < this->categoryArray.Size(); i++)
	{
		for (auto const& prop : this->categoryArray[i].properties)
		{
			prop->OnBeginFrame();
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
CategoryManager::OnFrame()
{
	for (IndexT i = 0; i < this->categoryArray.Size(); i++)
	{
		for (auto const& prop : this->categoryArray[i].properties)
		{
			prop->OnRender();
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
CategoryManager::OnEndFrame()
{
	for (IndexT i = 0; i < this->categoryArray.Size(); i++)
	{
		for (auto const& prop : this->categoryArray[i].properties)
		{
			prop->OnEndFrame();
		}
	}
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

//------------------------------------------------------------------------------
/**
*/
void
CategoryManager::BeginAddCategoryAttrs(Util::StringAtom categoryName)
{
    n_assert(!this->inBeginAddCategoryAttrs);
    this->addAttrCategoryIndex = catIndexMap[categoryName].id;
    this->inBeginAddCategoryAttrs = true;
}

//------------------------------------------------------------------------------
/**
*/
void
CategoryManager::AddCategoryAttr(const Game::AttributeId& attrId)
{
    n_assert(this->inBeginAddCategoryAttrs);
    Ptr<Game::Database> db = EntityManager::Instance()->GetWorldDatabase();
    Category& cat = this->categoryArray[this->addAttrCategoryIndex];
    db->AddColumn(cat.templateTable, attrId);
    db->AddColumn(cat.instanceTable, attrId);
}

//------------------------------------------------------------------------------
/**
*/
void
CategoryManager::AddProperty(const Ptr<Game::Property>& prop)
{
	n_assert(this->inBeginAddCategoryAttrs);
	this->categoryArray[this->addAttrCategoryIndex].properties.Append(prop);
	const_cast<CategoryId&>(prop->category) = CategoryId(this->addAttrCategoryIndex);
	prop->SetupExternalAttributes();
	prop->Init();
}

//------------------------------------------------------------------------------
/**
*/
void
CategoryManager::EndAddCategoryAttrs()
{
    n_assert(this->inBeginAddCategoryAttrs);
    this->inBeginAddCategoryAttrs = false;
}

} // namespace Game


