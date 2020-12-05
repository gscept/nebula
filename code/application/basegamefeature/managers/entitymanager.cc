//------------------------------------------------------------------------------
//  entitymanager.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "entitymanager.h"
#include "game/entity.h"
#include "blueprintmanager.h"
#include "game/api.h"

namespace Game
{

__ImplementSingleton(EntityManager)

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
    if (info.templateId != TemplateId::Invalid())
    {
        cmd.tid = info.templateId;
    }
    else
    {
        cmd.tid.blueprintId = info.blueprint.id;
        cmd.tid.templateId = Ids::InvalidId16;
    }

    if (!info.immediate)
    {
        state->allocQueue.Enqueue(std::move(cmd));
    }
    else
    {
        if (cmd.tid.templateId != Ids::InvalidId16)
        {
            EntityManager::Singleton->AllocateInstance(cmd.entity, cmd.tid);
        }
        else
        {
            EntityManager::Singleton->AllocateInstance(cmd.entity, (BlueprintId)cmd.tid.blueprintId);
        }
    }

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

    // NOTE: The order of the following loops are important!

    // Clean up any managed property instances.
    for (IndexT c = 0; c < state->categoryArray.Size(); c++)
    {
        MemDb::TableId tid = state->categoryArray[c].managedPropertyTable;
        state->worldDatabase->Clean(tid);
    }

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

        if (cmd.tid.templateId != Ids::InvalidId16)
        {
            EntityManager::Singleton->AllocateInstance(cmd.entity, cmd.tid);
        }
        else
        {
            EntityManager::Singleton->AllocateInstance(cmd.entity, (BlueprintId)cmd.tid.blueprintId);
        }
    }

    // Delete all remaining invalid instances
    Ptr<MemDb::Database> const& db = state->worldDatabase;

    for (IndexT c = 0; c < state->categoryArray.Size(); c++)
    {
        Category& cat = state->categoryArray[c];
        MemDb::Table& table = db->GetTable(cat.instanceTable);
        MemDb::ColumnIndex ownerColumnId = db->GetColumnId(cat.instanceTable, state->ownerId);

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

    Game::ReleaseAllOps();
}

//------------------------------------------------------------------------------
/**
*/
EntityManager::EntityManager()
{
    this->state.numEntities = 0;
    this->state.worldDatabase = MemDb::Database::Create();
    this->state.templateDatabase = MemDb::Database::Create();
    this->state.ownerId = MemDb::TypeRegistry::GetPropertyId("Owner"_atm);
}

//------------------------------------------------------------------------------
/**
*/
EntityManager::~EntityManager()
{
    this->state.worldDatabase = nullptr;
    this->state.templateDatabase = nullptr;
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
    for (int i = 0; i < info.properties.Size(); i++)
    {
        catHash.AddToHash(info.properties[i].id);
    }

    if (this->state.catIndexMap.Contains(catHash))
    {
        return this->state.catIndexMap[catHash];
    }

    Category cat;
    constexpr ushort NUM_PROPS = 256;
    PropertyId properties[NUM_PROPS];

    MemDb::TableCreateInfo tableInfo;
    tableInfo.name = info.name;
    if (info.properties[0] != this->state.ownerId)
    {
        // push owner id into the property array
        const SizeT tableSize = 1 + info.properties.Size();
        n_assert(tableSize < NUM_PROPS);
        tableInfo.numColumns = tableSize;
        tableInfo.columns = properties;

        // always add owner as first column
        properties[0] = this->state.ownerId;
        for (int i = 1; i < tableSize; i++)
        {
            properties[i] = info.properties[i - 1];
        }
    }
    else
    {
        const SizeT tableSize = info.properties.Size();
        tableInfo.numColumns = tableSize;
        tableInfo.columns = info.properties.begin();
    }

    // Create an instance table
    cat.instanceTable = this->state.worldDatabase->CreateTable(tableInfo);

    // Find all managed properties
    int numManaged = 0;
    for (int i = 0; i < info.properties.Size(); i++)
    {
        if ((MemDb::TypeRegistry::Flags(info.properties[i]) & PropertyFlags::PROPERTYFLAG_MANAGED) == PropertyFlags::PROPERTYFLAG_MANAGED)
        {
            properties[numManaged] = info.properties[i];
            numManaged++;
            n_assert(numManaged < NUM_PROPS);
        }
    }

    // Managed properties table
    if (numManaged > 0)
    {
        MemDb::TableCreateInfo managedTableInfo;
        managedTableInfo.name = "<MNGD>:" + info.name;
        managedTableInfo.columns = properties;
        managedTableInfo.numColumns = numManaged;
        cat.managedPropertyTable = this->state.worldDatabase->CreateTable(managedTableInfo);
    }
    else
        cat.managedPropertyTable = MemDb::TableId::Invalid();

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
    Game::Entity* owners = (Game::Entity*)this->state.worldDatabase->GetBuffer(cat.instanceTable, 0);
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
    Game::Entity* owners = (Game::Entity*)this->state.worldDatabase->GetBuffer(cat.instanceTable, 0);
    owners[mapping.instance.id] = entity;

    return mapping.instance;
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
EntityManager::AllocateInstance(Entity entity, TemplateId templateId)
{
    n_assert(IsValid(entity));
    
    if (Ids::Index(entity.id) < this->state.entityMap.Size() && this->state.entityMap[Ids::Index(entity.id)].instance != Game::InstanceId::Invalid())
    {
        n_warning("Entity already registered!\n");
        return InvalidIndex;
    }

    EntityMapping mapping = BlueprintManager::Instance()->Instantiate(templateId);
    this->state.entityMap[Ids::Index(entity.id)] = mapping;

    Category const& cat = this->GetCategory(mapping.category);
    // Just make sure the first column in always owner!
    n_assert(this->state.worldDatabase->GetColumnId(cat.instanceTable, this->state.ownerId) == 0);

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)this->state.worldDatabase->GetBuffer(cat.instanceTable, 0);
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

    if (cat.managedPropertyTable == MemDb::TableId::Invalid())
        this->state.worldDatabase->DeallocateRow(cat.instanceTable, instance.id);
    else
    {
        // migrate to managed property table so that we can allow the managers
        // to clean up any externally allocated resources.
        this->state.worldDatabase->MigrateInstance(cat.instanceTable, instance.id, cat.managedPropertyTable, false);
    }

    instance = InstanceId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
EntityManager::Migrate(Entity entity, CategoryId newCategory)
{
    EntityMapping mapping = GetEntityMapping(entity);
    Category const& oldCat = this->GetCategory(mapping.category);
    Category const& newCat = this->GetCategory(newCategory);
    InstanceId newInstance = this->state.worldDatabase->MigrateInstance(oldCat.instanceTable, mapping.instance.id, newCat.instanceTable);
    this->state.entityMap[Ids::Index(entity.id)] = { newCategory, newInstance };
    return newInstance;
}

//------------------------------------------------------------------------------
/**
    @param newInstances		Will be filled with the new instance ids in the destination category.
    @note	This assumes ALL entities in the entity array is of same category!
*/
void
EntityManager::Migrate(Util::Array<Entity> const& entities, CategoryId fromCategory, CategoryId newCategory, Util::FixedArray<IndexT>& newInstances)
{
    if (newInstances.Size() != entities.Size())
    {
        newInstances.SetSize(entities.Size());
    }

    Util::Array<IndexT> instances;
    SizeT const num = entities.Size();
    instances.Reserve(num);

    for (auto entity : entities)
    {
        EntityMapping mapping = GetEntityMapping(entity);
#ifdef NEBULA_DEBUG
        n_assert(mapping.category == fromCategory);
#endif // NEBULA_DEBUG
        instances.Append(mapping.instance.id);
    }

    Category const& oldCat = this->GetCategory(fromCategory);
    Category const& newCat = this->GetCategory(newCategory);

    GetWorldDatabase()->MigrateInstances(oldCat.instanceTable, instances, newCat.instanceTable, newInstances);

    for (IndexT i = 0; i < num; i++)
    {
        this->state.entityMap[Ids::Index(entities[i].id)] = { newCategory, (uint32_t)newInstances[i] };
    }
}

} // namespace Game
