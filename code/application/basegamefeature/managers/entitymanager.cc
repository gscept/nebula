//------------------------------------------------------------------------------
//  entitymanager.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "entitymanager.h"
#include "game/entity.h"
#include "blueprintmanager.h"

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
Ptr<MemDb::Database>
GetWorldDatabase()
{
	n_assert(EntityManager::HasInstance());
	return EntityManager::Singleton->state.worldDatabase;
}


//------------------------------------------------------------------------------
/**
*/
bool
CategoryExists(CategoryHash hash)
{
	n_assert(EntityManager::HasInstance());
	return EntityManager::Singleton->state.catIndexMap.Contains(hash);
}

//------------------------------------------------------------------------------
/**
*/
CategoryId const
GetCategoryId(CategoryHash hash)
{
	n_assert(EntityManager::HasInstance());
	n_assert(EntityManager::Singleton->state.catIndexMap.Contains(hash));
	return EntityManager::Singleton->state.catIndexMap[hash];
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
PropertyId const
GetPropertyId(Util::StringAtom name)
{
	return MemDb::TypeRegistry::GetDescriptor(name);
}

//------------------------------------------------------------------------------
/**
*/
void
AddProperty(Game::Entity const entity, PropertyId const pid)
{
	EntityManager::State& state = EntityManager::Singleton->state;
	EntityMapping mapping = GetEntityMapping(entity);
	Category const& cat = EntityManager::Singleton->GetCategory(mapping.category);
	CategoryHash newHash = cat.hash;
	newHash.AddToHash(pid.id);
	CategoryId newCategoryId;
	if (state.catIndexMap.Contains(newHash))
	{
		newCategoryId = state.catIndexMap[newHash];
	}
	else
	{
		CategoryCreateInfo info;
		auto const& cols = state.worldDatabase->GetTable(cat.instanceTable).columns.GetArray<0>();
		info.columns.SetSize(cols.Size());
		IndexT i;
		// Note: Skips owner column
		for (i = 0; i < cols.Size() - 1; ++i)
		{
			info.columns[i] = cols[i + 1];
		}
		info.columns[i] = pid;

#ifdef NEBULA_DEBUG
		info.name = cat.name + " + ";
		info.name += MemDb::TypeRegistry::GetDescription(pid)->name.AsString();
#endif

		newCategoryId = EntityManager::Singleton->CreateCategory(info);
	}

	EntityManager::Singleton->Migrate(entity, newCategoryId);
}

//------------------------------------------------------------------------------
/**
*/
BlueprintId const
GetBlueprintId(Util::StringAtom name)
{
	return BlueprintManager::GetBlueprintId(name);
}

//------------------------------------------------------------------------------
/**
*/
Dataset
Query(FilterSet const& filter)
{
	return EntityManager::Singleton->state.worldDatabase->Query(filter);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
GetNumInstances(CategoryId category)
{
	Ptr<MemDb::Database> db = GetWorldDatabase();
	MemDb::TableId tid = EntityManager::Instance()->GetCategory(category).instanceTable;
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
void
OnBeginFrame()
{
	n_assert(EntityManager::HasInstance());
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
OnFrame()
{
	n_assert(EntityManager::HasInstance());
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
OnEndFrame()
{
	n_assert(EntityManager::HasInstance());
	EntityManager::State* const state = &EntityManager::Singleton->state;

	// Clean up entities
	while (!state->deallocQueue.IsEmpty())
	{
		auto const cmd = state->deallocQueue.Dequeue();
		EntityMapping mapping = state->entityMap[Ids::Index(cmd.entity.id)];
		Category const& category = EntityManager::Singleton->GetCategory(mapping.category);
		EntityManager::Singleton->DeallocateInstance(cmd.entity);
	}

	// Allocate instances for new entities, reuse invalid instances if possible
	while (!state->allocQueue.IsEmpty())
	{
		auto const cmd = state->allocQueue.Dequeue();
		n_assert(IsValid(cmd.entity));

		if (cmd.info.templateId != TemplateId::Invalid())
		{
			EntityManager::Singleton->AllocateInstance(cmd.entity, cmd.info.blueprint, cmd.info.templateId);
		}
		else
		{
			EntityManager::Singleton->AllocateInstance(cmd.entity, cmd.info.blueprint);
		}
	}

	// Delete all remaining invalid instances
	Ptr<MemDb::Database> const& db = state->worldDatabase;

	for (IndexT c = 0; c < state->categoryArray.Size(); c++)
	{
		Category& cat = state->categoryArray[c];
		MemDb::Table& table = db->GetTable(cat.instanceTable);
		MemDb::ColumnId ownerColumnId = db->GetColumnId(cat.instanceTable, state->ownerId);

		// defragment the table. Any instances that has been deleted will be swap'n'popped,
		// which means we need to update the entity mapping.
		// The move callback is signaled BEFORE the swap has happened.
		auto* const map = &state->entityMap;
		SizeT numErased = db->Defragment(cat.instanceTable, [map, cat, &ownerColumnId, &table](InstanceId from, InstanceId to)
		{
			Game::Entity fromEntity = ((Game::Entity*)(table.columns.Get<1>(ownerColumnId.id)))[from.id].id;
			Game::Entity toEntity = ((Game::Entity*)(table.columns.Get<1>(ownerColumnId.id)))[to.id].id;
			if (!IsValid(fromEntity))
			{
				// we need to add this instances new index to the to the freeids list, since it's been deleted.
				// the 'from' instance will be swapped with the 'to' instance, so we just add the 'to' id to the list;
				// and it will automatically be defragged
				table.freeIds.Append(to.id);
			}
			else
			{
				(*map)[Ids::Index(fromEntity.id)].instance = to;
				(*map)[Ids::Index(to.id)].instance = from;
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
	this->state.worldDatabase = MemDb::Database::Create();
	this->state.ownerId = MemDb::TypeRegistry::GetDescriptor("Owner"_atm);
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
	EntityManager::Singleton = n_new(EntityManager);

	ManagerAPI api;
	api.OnBeginFrame	= &OnBeginFrame;
	api.OnFrame			= &OnFrame;
	api.OnEndFrame		= &OnEndFrame;
	api.OnDeactivate	= &Destroy;
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
EntityManager::CreateCategory(CategoryCreateInfo const& info)
{
	CategoryHash catHash;
	for (int i = 0; i < info.columns.Size(); i++)
	{
		catHash.AddToHash(info.columns[i].id);
	}

	if (this->state.catIndexMap.Contains(catHash))
	{
		return this->state.catIndexMap[catHash];
	}

	MemDb::TableCreateInfo tableInfo;
	tableInfo.name = info.name;
	const SizeT tableSize = info.columns.Size() + 1;
	tableInfo.columns.SetSize(tableSize);

	// always add owner as first column
	tableInfo.columns[0] = this->state.ownerId;
	for (int i = 1; i < tableSize; i++)
	{
		n_assert2(info.columns[i - 1] != PropertyId::Invalid(), "ERROR: Invalid property in CategoryCreateInfo!\n");
		tableInfo.columns[i] = info.columns[i - 1];
	}
	
	Category cat;
	// Create an instance table
	cat.instanceTable = this->state.worldDatabase->CreateTable(tableInfo);
	cat.hash = catHash;

#ifdef NEBULA_DEBUG
	cat.name = info.name;
#endif

	CategoryId cid = this->state.categoryArray.Size();

	this->state.catIndexMap.Add(catHash, cid.id);
	this->state.categoryArray.Append(cat);

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
	n_assert(this->state.worldDatabase->GetColumnId(cat.instanceTable, this->state.ownerId) == 0);

	// Set the owner of this instance
	Game::Entity* owners = (Game::Entity*) * this->state.worldDatabase->GetPersistantBuffer(cat.instanceTable, 0);
	owners[instance.id] = entity;

	return instance;
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
EntityManager::AllocateInstance(Entity entity, BlueprintId blueprint)
{
	n_assert(IsValid(entity));

	if (Ids::Index(entity.id) < this->state.entityMap.Size() && this->state.entityMap[Ids::Index(entity.id)].instance != Game::InstanceId::Invalid())
	{
		n_warning("Entity already registered!\n");
		return InvalidIndex;
	}

	EntityMapping mapping = BlueprintManager::Instance()->Instantiate(blueprint);
	this->state.entityMap[Ids::Index(entity.id)] = mapping;

	Category const& cat = this->GetCategory(mapping.category);
	// Just make sure the first column in always owner!
	n_assert(this->state.worldDatabase->GetColumnId(cat.instanceTable, this->state.ownerId) == 0);

	// Set the owner of this instance
	Game::Entity* owners = (Game::Entity*) * this->state.worldDatabase->GetPersistantBuffer(cat.instanceTable, 0);
	owners[mapping.instance.id] = entity;

	return mapping.instance;
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
EntityManager::AllocateInstance(Entity entity, BlueprintId blueprint, TemplateId templateId)
{
	n_assert(IsValid(entity));
	
	if (Ids::Index(entity.id) < this->state.entityMap.Size() && this->state.entityMap[Ids::Index(entity.id)].instance != Game::InstanceId::Invalid())
	{
		n_warning("Entity already registered!\n");
		return InvalidIndex;
	}

	EntityMapping mapping = BlueprintManager::Instance()->Instantiate(blueprint, templateId);
	this->state.entityMap[Ids::Index(entity.id)] = mapping;

	Category const& cat = this->GetCategory(mapping.category);
	// Just make sure the first column in always owner!
	n_assert(this->state.worldDatabase->GetColumnId(cat.instanceTable, this->state.ownerId) == 0);

	// Set the owner of this instance
	Game::Entity* owners = (Game::Entity*) *this->state.worldDatabase->GetPersistantBuffer(cat.instanceTable, 0);
	owners[mapping.instance.id] = entity;

	return mapping.instance;
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
InstanceId
EntityManager::Migrate(Entity entity, CategoryId newCategory)
{
	EntityManager::State& state = EntityManager::Singleton->state;
	EntityMapping mapping = GetEntityMapping(entity);
	Category const& oldCat = EntityManager::Singleton->GetCategory(mapping.category);
	Category const& newCat = EntityManager::Singleton->GetCategory(newCategory);
	InstanceId newInstance = state.worldDatabase->MigrateInstance(oldCat.instanceTable, mapping.instance.id, newCat.instanceTable);
	state.entityMap[Ids::Index(entity.id)] = { newCategory, newInstance };
	return newInstance;
}

} // namespace Game
