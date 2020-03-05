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
SizeT
GetNumInstances(CategoryId category)
{
	Ptr<Game::Db::Database> db = Game::EntityManager::Instance()->GetWorldDatabase();
	Db::TableId tid = CategoryManager::Instance()->GetCategory(category).instanceTable;
	return db->GetNumRows(tid);
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
GetInstanceId(Entity entity)
{
	Ptr<CategoryManager> mgr = CategoryManager::Instance();
	auto mapping = mgr->GetEntityMapping(entity);
	return mapping.instance;
}


//------------------------------------------------------------------------------
/**
*/
CategoryManager::CategoryManager() :
    inBeginAddCategoryAttrs(false),
    addAttrCategoryIndex(0)
{
	__ConstructSingleton;
	_setup_grouped_timer(CategoryManagerOnBeginFrame, "Game Subsystem");
}

//------------------------------------------------------------------------------
/**
*/
CategoryManager::~CategoryManager()
{
	__DestructSingleton;
	_discard_timer(CategoryManagerOnBeginFrame);
}

//------------------------------------------------------------------------------
/**
*/
void
CategoryManager::OnBeginFrame()
{
	_start_timer(CategoryManagerOnBeginFrame);

	for (IndexT i = 0; i < this->categoryArray.Size(); i++)
	{
		for (auto const& prop : this->categoryArray[i].properties)
		{
			prop->OnBeginFrame();
		}
	}

	_stop_timer(CategoryManagerOnBeginFrame);
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

	// Delete all invalid instances
	Ptr<Game::Db::Database> db = EntityManager::Instance()->GetWorldDatabase();

	for (IndexT c = 0; c < this->categoryArray.Size(); c++)
	{
		Category& cat = this->categoryArray[c];
		
		Game::Db::Table& table = db->GetTable(cat.instanceTable);
		Db::ColumnId ownerColumnId = db->GetColumnId(cat.instanceTable, Attr::Owner::Id());

		// First, deactivate all deleted instances
		for (auto const& prop : cat.properties)
		{
			for (IndexT id : table.freeIds)
				prop->OnDeactivate(id);
		}

		// Now, defragment the table. Any instances that has been deleted will be swap'n'popped,
		// which means we need to update the entity mapping.
		// The move callback is signaled BEFORE the swap has happened.
		SizeT numErased = db->Defragment(cat.instanceTable, [this, &ownerColumnId, &table](InstanceId from, InstanceId to)
		{
			Game::Entity fromEntity = ((Game::Entity*)(table.columns.Get<1>(ownerColumnId.id)))[from.id].id;
			Game::Entity toEntity = ((Game::Entity*)(table.columns.Get<1>(ownerColumnId.id)))[to.id].id;
			if (!Game::EntityManager::Instance()->IsValid(fromEntity))
			{
				// we need to add this instances new index to the to the freeids list, since it's been deleted
				// the 'from' instance will be swapped with the 'to' instance, so we just add the to id to the list;
				// and it will automatically be defragged
				table.freeIds.Append(to.id);
			}
			else
			{
				this->entityMap[Ids::Index(fromEntity.id)].instance = to;
				this->entityMap[Ids::Index(to.id)].instance = from;
			}
		});
	}
}

//------------------------------------------------------------------------------
/**
*/
void
CategoryManager::AddCategory(CategoryCreateInfo const& info)
{
	Db::TableCreateInfo tableInfo;
	tableInfo.name = info.name;
	
	Ptr<Game::Db::Database> db = EntityManager::Instance()->GetWorldDatabase();

	Category cat;
	cat.name = info.name;
	// Create an instance table
	cat.instanceTable = db->CreateTable(tableInfo);
	// Create a identical table that can hold templates of this category
	cat.templateTable = db->CreateTable(tableInfo);

	this->catIndexMap.Add(cat.name, this->categoryArray.Size());
	this->categoryArray.Append(cat);

	this->BeginAddCategoryAttrs(info.name);
	// Add the owner attribute to the table.
	// NOTE: This is always assumed to be the first column!
	SetupAttr(Attr::Runtime::OwnerId);
	for (auto col : info.columns)
	{
		SetupAttr(col);
	}
	this->EndAddCategoryAttrs();
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
CategoryManager::AllocateInstance(Entity entity, CategoryId category)
{
	n_assert(EntityManager::Instance()->IsValid(entity));
	n_assert(category < this->categoryArray.Size());

	if (Ids::Index(entity.id) < this->entityMap.Size() && this->entityMap[Ids::Index(entity.id)].instance != Game::InstanceId::Invalid())
	{
		n_warning("Entity already registered!\n");
		return InvalidIndex;
	}

	Category& cat = this->categoryArray[category.id];
	Ptr<Game::Db::Database> db = EntityManager::Instance()->GetWorldDatabase();
	InstanceId instance = db->AllocateRow(cat.instanceTable);

	if (this->entityMap.Size() <= Ids::Index(entity.id))
	{
		this->entityMap.Grow();
		// Just make sure we don't assert on any entity id
		this->entityMap.SetSize(this->entityMap.Capacity());
	}
	this->entityMap[Ids::Index(entity.id)] = { category, instance };

	// Just make sure the first column in always owner!
	n_assert(db->GetColumnId(cat.instanceTable, Attr::Owner::Id()) == 0);

	// Set the owner of this instance
	Game::Entity* owners = (Game::Entity*)*db->GetPersistantBuffer(cat.instanceTable, 0);
	owners[instance.id] = entity;

	return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
CategoryManager::DeallocateInstance(Entity entity)
{
	CategoryId category = this->entityMap[Ids::Index(entity.id)].category;
	n_assert(category < this->categoryArray.Size());

	Category& cat = this->categoryArray[category.id];
	Ptr<Game::Db::Database> db = EntityManager::Instance()->GetWorldDatabase();

	InstanceId instance = this->entityMap[Ids::Index(entity.id)].instance;
	n_assert(instance != Game::InstanceId::Invalid());

	db->DeallocateRow(cat.instanceTable, instance.id);
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
    Ptr<Game::Db::Database> db = EntityManager::Instance()->GetWorldDatabase();
    Category& cat = this->categoryArray[this->addAttrCategoryIndex];
    db->AddColumn(cat.templateTable, attrId);
    db->AddColumn(cat.instanceTable, attrId);

	if (!attrId.GetCategoryTable()->Contains(this->addAttrCategoryIndex))
	{
		void** buf = db->GetPersistantBuffer(cat.instanceTable, db->GetColumnId(cat.instanceTable, attrId));
		n_assert(!attrId.GetCategoryTable()->Contains(this->addAttrCategoryIndex));
		attrId.GetCategoryTable()->Add(this->addAttrCategoryIndex, buf);
		void** test = ((*attrId.GetCategoryTable())[this->addAttrCategoryIndex]);
		void* test2 = *test;
		n_assert(test != nullptr);
		n_assert(test2 != nullptr);
	}
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


