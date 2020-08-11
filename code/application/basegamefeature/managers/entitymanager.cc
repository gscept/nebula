//------------------------------------------------------------------------------
//  entitymanager.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "entitymanager.h"
#include "game/entity.h"

namespace Attr
{
__DefineAttribute(Owner);
}

namespace Game
{

__ImplementSingleton(EntityManager)

//------------------------------------------------------------------------------
/**
*/
bool
IsValid(Entity e)
{
	n_assert(EntityManager::HasInstance());
	return EntityManager::Singleton->state.pool.IsValid(e.id);
}

//------------------------------------------------------------------------------
/**
*/
bool
IsActive(Entity e)
{
	n_assert(EntityManager::HasInstance());
	n_assert(IsValid(e));
	return EntityManager::Singleton->state.entityMap[Ids::Index(e.id)].instance != InstanceId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
uint
GetNumEntities()
{
	n_assert(EntityManager::HasInstance());
	return EntityManager::Singleton->state.numEntities;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<Game::Db::Database>
GetWorldDatabase()
{
	n_assert(EntityManager::HasInstance());
	return EntityManager::Singleton->state.worldDatabase;
}


//------------------------------------------------------------------------------
/**
*/
bool
CategoryExists(Util::StringAtom name)
{
	n_assert(EntityManager::HasInstance());
	return EntityManager::Singleton->state.catIndexMap.Contains(name);
}

//------------------------------------------------------------------------------
/**
*/
CategoryId const
GetCategoryId(Util::StringAtom name)
{
	n_assert(EntityManager::HasInstance());
	n_assert(EntityManager::Singleton->state.catIndexMap.Contains(name));
	return EntityManager::Singleton->state.catIndexMap[name];
}

//------------------------------------------------------------------------------
/**
*/
EntityMapping
GetEntityMapping(Game::Entity entity)
{
	n_assert(EntityManager::HasInstance());
	return EntityManager::Singleton->state.entityMap[Ids::Index(entity.id)];
}

//------------------------------------------------------------------------------
/**
*/
SizeT
GetNumInstances(CategoryId category)
{
	Ptr<Game::Db::Database> db = GetWorldDatabase();
	Db::TableId tid = EntityManager::Instance()->GetCategory(category).instanceTable;
	return db->GetNumRows(tid);
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
GetInstanceId(Entity entity)
{
	return GetEntityMapping(entity).instance;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
CreateEntity(EntityCreateInfo const& info)
{
	n_assert(EntityManager::HasInstance());
	EntityManager::State* const state = &EntityManager::Singleton->state;

	Entity entity;
	state->pool.Allocate(entity.id);
	state->numEntities++;

	// Make sure the entitymap can contain this entity
	if (state->entityMap.Size() <= Ids::Index(entity.id))
	{
		state->entityMap.Grow();
		state->entityMap.Resize(state->entityMap.Capacity());
	}

	EntityManager::State::AllocateInstanceCommand cmd;
	cmd.entity = entity;
	cmd.info = info;

	state->allocQueue.Enqueue(std::move(cmd));

	return entity;
}

//------------------------------------------------------------------------------
/**
*/
void
DeleteEntity(Game::Entity entity)
{
	n_assert(IsValid(entity));
	n_assert(EntityManager::HasInstance());
	EntityManager::State* const state = &EntityManager::Singleton->state;

	// make sure we're not trying to dealloc an instance that does not exist
	n_assert2(IsActive(entity), "Cannot delete and entity before it has been instantiated!\n");

	state->pool.Deallocate(entity.id);

	EntityManager::State::DeallocInstanceCommand cmd;
	cmd.entity = entity;

	state->deallocQueue.Enqueue(std::move(cmd));

	state->numEntities--;
}

//------------------------------------------------------------------------------
/**
*/
Dataset
Query(FilterSet const& filterset)
{
	n_assert(EntityManager::HasInstance());
	EntityManager::State* const state = &EntityManager::Singleton->state;

	Dataset set;
	set.filter = filterset;

	state->worldDatabase;

	SizeT const numCats = state->categoryArray.Size();
	for (IndexT cid = 0; cid < numCats; cid++)
	{
		bool valid = true;
		for (auto attrid : filterset.inclusive)
		{
			if (!attrid.GetCategoryTable()->Contains(cid))
			{
				valid = false;
				break;
			}
		}

		if (valid)
		{
			for (auto attrid : filterset.exclusive)
			{
				if (attrid.GetCategoryTable()->Contains(cid))
				{
					valid = false;
					break;
				}
			}
		}

		if (valid)
		{
			Util::ArrayStack<void*, 16> buffers;
			buffers.Reserve(filterset.inclusive.Size());

			Db::TableId const tid = state->categoryArray[cid].instanceTable;
			Db::Table const& tbl = state->worldDatabase->GetTable(tid);

			IndexT i = 0;
			for (auto attrid : filterset.inclusive)
			{
				Db::ColumnId colId = state->worldDatabase->GetColumnId(tid, attrid);
				buffers.Append(tbl.columns.Get<1>(colId.id));
			}

			Dataset::View view = {
				cid,
				GetNumInstances(cid),
				std::move(buffers)
			};

			set.categories.Append(std::move(view));
		}
	}

	return set;
}


//------------------------------------------------------------------------------
/**
*/
void
OnBeginFrame()
{
	n_assert(EntityManager::HasInstance());
	EntityManager::State* const state = &EntityManager::Singleton->state;

	for (IndexT i = 0; i < state->categoryArray.Size(); i++)
	{
		for (auto const& prop : state->categoryArray[i].properties)
		{
			prop->OnBeginFrame();
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
OnFrame()
{
	n_assert(EntityManager::HasInstance());
	EntityManager::State* const state = &EntityManager::Singleton->state;

	for (IndexT i = 0; i < state->categoryArray.Size(); i++)
	{
		for (auto const& prop : state->categoryArray[i].properties)
		{
			prop->OnRender();
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
OnEndFrame()
{
	n_assert(EntityManager::HasInstance());
	EntityManager::State* const state = &EntityManager::Singleton->state;

	for (IndexT i = 0; i < state->categoryArray.Size(); i++)
	{
		for (auto const& prop : state->categoryArray[i].properties)
		{
			prop->OnEndFrame();
		}
	}

	// Clean up entities
	while (!state->deallocQueue.IsEmpty())
	{
		auto const cmd = state->deallocQueue.Dequeue();
		EntityMapping mapping = state->entityMap[Ids::Index(cmd.entity.id)];
		Category const& category = EntityManager::Singleton->GetCategory(mapping.category);
		for (auto const& prop : category.properties)
		{
			prop->OnDeactivate(mapping.instance);
		}

		EntityManager::Singleton->DeallocateInstance(cmd.entity);
	}

	// Allocate instances for new entities, reuse invalid instances if possible
	while (!state->allocQueue.IsEmpty())
	{
		auto const cmd = state->allocQueue.Dequeue();
		n_assert(IsValid(cmd.entity));

		CategoryId const cid = cmd.info.category;
		InstanceId const instance = EntityManager::Singleton->AllocateInstance(cmd.entity, cid);
		Category const& category = EntityManager::Singleton->GetCategory(cid);

		// Set attributes before activating
		// TODO: We should probably create a Game namespace abstraction for setting an attribute value by AttributeId and AttributeValue
		SizeT const numAttrs = cmd.info.attributes.size();
		for (IndexT i = 0; i < numAttrs; ++i)
		{
			//Db::Table const& table = db->GetTable(category.instanceTable);
			Db::ColumnId columnId = state->worldDatabase->GetColumnId(category.instanceTable, cmd.info.attributes.begin()[i].Key());
			n_assert(columnId != Db::ColumnId::Invalid());
			state->worldDatabase->Set(category.instanceTable, columnId, instance.id, cmd.info.attributes.begin()[i].Value());
		}

		for (auto const& prop : category.properties)
		{
			prop->OnActivate(instance);
		}
	}

	// Delete all remaining invalid instances
	Ptr<Game::Db::Database> const& db = state->worldDatabase;

	for (IndexT c = 0; c < state->categoryArray.Size(); c++)
	{
		Category& cat = state->categoryArray[c];
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
		auto* const map = &state->entityMap;
		SizeT numErased = db->Defragment(cat.instanceTable, [map, cat, &ownerColumnId, &table](InstanceId from, InstanceId to)
		{
			Game::Entity fromEntity = ((Game::Entity*)(table.columns.Get<1>(ownerColumnId.id)))[from.id].id;
			Game::Entity toEntity = ((Game::Entity*)(table.columns.Get<1>(ownerColumnId.id)))[to.id].id;
			if (!IsValid(fromEntity))
			{
				// we need to add this instances new index to the to the freeids list, since it's been deleted
				// the 'from' instance will be swapped with the 'to' instance, so we just add the to id to the list;
				// and it will automatically be defragged
				table.freeIds.Append(to.id);
			}
			else
			{
				(*map)[Ids::Index(fromEntity.id)].instance = to;
				(*map)[Ids::Index(to.id)].instance = from;

				// Let the properties react to any moved instances.
				// TODO: this might be really expensive... We should consider letting each property register
				//       a callback for this instead, just to not waste time on empty function calls
				for (auto const& prop : cat.properties)
				{
					for (IndexT id : table.freeIds)
						prop->OnInstanceMoved(from, to);
				}
			}
		});
	}
}

//------------------------------------------------------------------------------
/**
*/
EntityManager::EntityManager()
{
	this->state.numEntities = 0;
	this->state.inBeginAddCategoryAttrs = false;
	this->state.addAttrCategoryIndex = 0;
	this->state.worldDatabase = Game::Db::Database::Create();
}

//------------------------------------------------------------------------------
/**
*/
EntityManager::~EntityManager()
{
	this->state.worldDatabase = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
ManagerAPI
EntityManager::Create()
{
	n_assert(!EntityManager::HasInstance());

	// FIXME: this became a bit convoluted... maybe we should just move the entire state to this .cc file and let the methods just be in another namespace (namespace EntityManager)
	EntityManager::Singleton = n_new(EntityManager);

	ManagerAPI api;
	api.OnBeginFrame	= &OnBeginFrame;
	api.OnFrame			= &OnFrame;
	api.OnEndFrame		= &OnEndFrame;
	return api;
}

//------------------------------------------------------------------------------
/**
*/
void
EntityManager::Destroy()
{
	n_delete(EntityManager::Singleton);
	EntityManager::Singleton = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
CategoryId
EntityManager::AddCategory(CategoryCreateInfo const& info)
{
	Db::TableCreateInfo tableInfo;
	tableInfo.name = info.name;

	Category cat;
	cat.name = info.name;
	// Create an instance table
	cat.instanceTable = this->state.worldDatabase->CreateTable(tableInfo);
	// Create a identical table that can hold templates of this category
	cat.templateTable = this->state.worldDatabase->CreateTable(tableInfo);

	CategoryId const cid = this->state.categoryArray.Size();
	this->state.catIndexMap.Add(cat.name, cid);
	this->state.categoryArray.Append(cat);

	this->BeginAddCategoryAttrs(info.name);
	// Add the owner attribute to the table.
	// NOTE: This is always assumed to be the first column!
	this->AddCategoryAttr(Attr::Runtime::OwnerId);
	for (auto col : info.columns)
	{
		this->AddCategoryAttr(col);
	}
	this->EndAddCategoryAttrs();

	return cid;
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
EntityManager::AllocateInstance(Entity entity, CategoryId category)
{
	n_assert(IsValid(entity));
	n_assert(category < this->state.categoryArray.Size());

	if (Ids::Index(entity.id) < this->state.entityMap.Size() && this->state.entityMap[Ids::Index(entity.id)].instance != Game::InstanceId::Invalid())
	{
		n_warning("Entity already registered!\n");
		return InvalidIndex;
	}

	Category& cat = this->state.categoryArray[category.id];
	InstanceId instance = this->state.worldDatabase->AllocateRow(cat.instanceTable);

	this->state.entityMap[Ids::Index(entity.id)] = { category, instance };

	// Just make sure the first column in always owner!
	n_assert(this->state.worldDatabase->GetColumnId(cat.instanceTable, Attr::Owner::Id()) == 0);

	// Set the owner of this instance
	Game::Entity* owners = (Game::Entity*) * this->state.worldDatabase->GetPersistantBuffer(cat.instanceTable, 0);
	owners[instance.id] = entity;

	return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
EntityManager::DeallocateInstance(Entity entity)
{
	CategoryId const category = this->state.entityMap[Ids::Index(entity.id)].category;
	n_assert(category < this->state.categoryArray.Size());

	Category& cat = this->state.categoryArray[category.id];
	InstanceId& instance = this->state.entityMap[Ids::Index(entity.id)].instance;
	
	n_assert(instance != Game::InstanceId::Invalid());

	this->state.worldDatabase->DeallocateRow(cat.instanceTable, instance.id);

	instance = InstanceId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
void
EntityManager::BeginAddCategoryAttrs(Util::StringAtom categoryName)
{
	n_assert(!this->state.inBeginAddCategoryAttrs);
	this->state.addAttrCategoryIndex = this->state.catIndexMap[categoryName].id;
	this->state.inBeginAddCategoryAttrs = true;
}

//------------------------------------------------------------------------------
/**
*/
void
EntityManager::AddCategoryAttr(const Game::AttributeId& attrId)
{
	n_assert(this->state.inBeginAddCategoryAttrs);
	Category& cat = this->state.categoryArray[this->state.addAttrCategoryIndex];
	this->state.worldDatabase->AddColumn(cat.templateTable, attrId);
	this->state.worldDatabase->AddColumn(cat.instanceTable, attrId);

	if (!attrId.GetCategoryTable()->Contains(this->state.addAttrCategoryIndex))
	{
		void** buf = this->state.worldDatabase->GetPersistantBuffer(cat.instanceTable, this->state.worldDatabase->GetColumnId(cat.instanceTable, attrId));
		n_assert(!attrId.GetCategoryTable()->Contains(this->state.addAttrCategoryIndex));
		attrId.GetCategoryTable()->Add(this->state.addAttrCategoryIndex, buf);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
EntityManager::AddProperty(const Ptr<Game::Property>& prop)
{
	n_assert(this->state.inBeginAddCategoryAttrs);
	this->state.categoryArray[this->state.addAttrCategoryIndex].properties.Append(prop);
	const_cast<CategoryId&>(prop->category) = CategoryId(this->state.addAttrCategoryIndex);
	prop->SetupExternalAttributes();
	prop->Init();
}

//------------------------------------------------------------------------------
/**
*/
void
EntityManager::EndAddCategoryAttrs()
{
	n_assert(this->state.inBeginAddCategoryAttrs);
	this->state.inBeginAddCategoryAttrs = false;
}

} // namespace Game
