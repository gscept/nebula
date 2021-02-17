//------------------------------------------------------------------------------
//  @file entitypool.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "entitypool.h"
#include "gameserver.h"
#include "basegamefeature/managers/blueprintmanager.h"

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
EntityPool::EntityPool()
{
    this->generations.Reserve(1024);
    this->freeIds.Reserve(2048);
}

//------------------------------------------------------------------------------
/**
*/
bool
EntityPool::Allocate(Entity& e)
{
    // make sure we don't run out of generations too fast
    // by always deallocating at least n entities before recycling
    if (this->freeIds.Size() < 1024)
    {
        this->generations.Append(0);
        e.index = this->generations.Size() - 1;
        n_assert2(e.index < 0x003FFFFF, "index overflow");
        e.generation = 0;
        return false;
    }
    else
    {
        uint32_t const index = this->freeIds.Dequeue();
        e.index = index;
        e.generation = this->generations[index];
        return true;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
EntityPool::Deallocate(Entity e)
{
    n_assert2(this->IsValid(e), "Tried to deallocate invalid/destroyed entity!");
    this->freeIds.Enqueue(e.index);
#if NEBULA_DEBUG
    // if you get this warning, you might want to consider reserving more bits for the generation.
    n_warn2(this->generations[e.index] <= 0x3FF, "Entity generation overflow!");
#endif
    this->generations[e.index]++;

}

//------------------------------------------------------------------------------
/**
*/
bool
EntityPool::IsValid(Entity e) const
{
    return e.index < (uint32_t)this->generations.Size() &&
        e.generation == this->generations[e.index];
}

//------------------------------------------------------------------------------
/**
*/
World::World() :
    numEntities(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
World::~World()
{
    this->db = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
CategoryId
World::CreateCategory(CategoryCreateInfo const& info)
{
    MemDb::TableSignature signature(info.properties);

    CategoryId categoryId = this->db->FindTable(signature);
    if (categoryId != CategoryId::Invalid())
    {
        return categoryId;
    }

    constexpr ushort NUM_PROPS = 256;
    PropertyId properties[NUM_PROPS];

    MemDb::TableCreateInfo tableInfo;
    tableInfo.name = info.name;
    if (info.properties[0] != GameServer::Singleton->state.ownerId)
    {
        // push owner id into the property array
        const SizeT tableSize = 1 + info.properties.Size();
        n_assert(tableSize < NUM_PROPS);
        tableInfo.numProperties = tableSize;
        tableInfo.properties = properties;

        // always add owner as first column
        properties[0] = GameServer::Singleton->state.ownerId;
        for (int i = 1; i < tableSize; i++)
        {
            properties[i] = info.properties[i - 1];
        }
    }
    else
    {
        const SizeT tableSize = info.properties.Size();
        tableInfo.numProperties = tableSize;
        tableInfo.properties = info.properties.begin();
    }

    // Create an instance table
    categoryId = this->db->CreateTable(tableInfo);

    // "Prefilter" the processors with the new table (insert the table in the cache that accepts it)
    this->CacheTable(categoryId, this->db->GetTableSignature(categoryId));

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
        // Create a decay table
        MemDb::TableCreateInfo managedTableInfo;
        managedTableInfo.name = "<MNGD>:" + info.name;
        managedTableInfo.properties = properties;
        managedTableInfo.numProperties = numManaged;
        CategoryId decayId = this->db->CreateTable(managedTableInfo);
        this->CacheTable(decayId, this->db->GetTableSignature(decayId));
        this->categoryDecayMap.Add(categoryId, decayId);
    }

    return categoryId;
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
World::AllocateInstance(Entity entity, CategoryId category)
{
    n_assert(this->pool.IsValid(entity));
    
    if (entity.index < this->entityMap.Size() && this->entityMap[entity.index].instance != Game::InstanceId::Invalid())
    {
        n_warning("Entity already registered!\n");
        return InvalidIndex;
    }

    InstanceId instance = this->db->AllocateRow(category);

    this->entityMap[entity.index] = { category, instance };

    // Just make sure the first column in always owner!
    n_assert(this->db->GetColumnId(category, GameServer::Singleton->state.ownerId) == 0);

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)this->db->GetBuffer(category, 0);
    owners[instance.id] = entity;

    return instance;
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
World::AllocateInstance(Entity entity, BlueprintId blueprint)
{
    n_assert(this->pool.IsValid(entity));

    if (entity.index < this->entityMap.Size() && this->entityMap[entity.index].instance != Game::InstanceId::Invalid())
    {
        n_warning("Entity already registered!\n");
        return InvalidIndex;
    }

    EntityMapping mapping = BlueprintManager::Instance()->Instantiate(this, blueprint);
    this->entityMap[entity.index] = mapping;

    // Just make sure the first column in always owner!
    n_assert(this->db->GetColumnId(mapping.category, GameServer::Singleton->state.ownerId) == 0);

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)this->db->GetBuffer(mapping.category, 0);
    owners[mapping.instance.id] = entity;

    return mapping.instance;
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
World::AllocateInstance(Entity entity, TemplateId templateId)
{
    n_assert(this->pool.IsValid(entity));

    if (entity.index < this->entityMap.Size() && this->entityMap[entity.index].instance != Game::InstanceId::Invalid())
    {
        n_warning("Entity instance already allocated!\n");
        return InvalidIndex;
    }

    EntityMapping mapping = BlueprintManager::Instance()->Instantiate(this, templateId);
    this->entityMap[entity.index] = mapping;

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)this->db->GetBuffer(mapping.category, 0);
    owners[mapping.instance.id] = entity;

    return mapping.instance;
}

//------------------------------------------------------------------------------
/**
*/
void
World::DeallocateInstance(Entity entity)
{
    CategoryId const category = this->entityMap[entity.index].category;
    InstanceId& instance = this->entityMap[entity.index].instance;

    n_assert(instance != Game::InstanceId::Invalid());

    IndexT decayIndex = this->categoryDecayMap.FindIndex(category);
    if (decayIndex == InvalidIndex)
        this->db->DeallocateRow(category, instance.id);
    else
    {
        // migrate to managed property table so that we can allow the managers
        // to clean up any externally allocated resources.
        this->db->MigrateInstance(category, instance.id, this->categoryDecayMap.ValueAtIndex(category, decayIndex), false);
    }

    instance = InstanceId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
void
World::DeallocateInstance(CategoryId category, InstanceId instance)
{
    n_assert(instance != Game::InstanceId::Invalid());

    IndexT decayIndex = this->categoryDecayMap.FindIndex(category);
    if (decayIndex == InvalidIndex)
        this->db->DeallocateRow(category, instance.id);
    else
    {
        // migrate to managed property table so that we can allow the managers
        // to clean up any externally allocated resources.
        this->db->MigrateInstance(category, instance.id, this->categoryDecayMap.ValueAtIndex(category, decayIndex), false);
    }
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
World::Migrate(Entity entity, CategoryId newCategory)
{
    EntityMapping mapping = GetEntityMapping(entity);
    InstanceId newInstance = this->db->MigrateInstance(mapping.category, mapping.instance.id, newCategory, false);
    DefragmentCategoryInstances(mapping.category);
    this->entityMap[entity.index] = { newCategory, newInstance };
    return newInstance;
}

//------------------------------------------------------------------------------
/**
    @param newInstances     Will be filled with the new instance ids in the destination category.
    @note   This assumes ALL entities in the entity array is of same category!
*/
void
World::Migrate(Util::Array<Entity> const& entities, CategoryId fromCategory, CategoryId newCategory, Util::FixedArray<IndexT>& newInstances)
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

    this->db->MigrateInstances(fromCategory, instances, newCategory, newInstances, false);
    DefragmentCategoryInstances(fromCategory);
    for (IndexT i = 0; i < num; i++)
    {
        this->entityMap[entities[i].index] = { newCategory, (uint32_t)newInstances[i] };
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::RegisterProcessor(std::initializer_list<ProcessorHandle> handles)
{
    for (auto handle : handles)
    {
        ProcessorInfo const& info = Game::GameServer::Instance()->GetProcessorInfo(handle);

        // Setup frame callbacks
        if (info.OnBeginFrame != nullptr)
            this->onBeginFrameCallbacks.Append({ handle, info.filter, info.OnBeginFrame });

        if (info.OnFrame != nullptr)
            this->onFrameCallbacks.Append({ handle, info.filter, info.OnFrame });

        if (info.OnEndFrame != nullptr)
            this->onEndFrameCallbacks.Append({ handle, info.filter, info.OnEndFrame });

        if (info.OnLoad != nullptr)
            this->onLoadCallbacks.Append({ handle, info.filter, info.OnLoad });

        if (info.OnSave != nullptr)
            this->onSaveCallbacks.Append({ handle, info.filter, info.OnSave });
    }
}

//------------------------------------------------------------------------------
/**
    @todo When cleaning up the db, erase all tables from the cache.
*/
void
World::CacheTable(MemDb::TableId tid, MemDb::TableSignature signature)
{
    // this is just to compress the code a bit
    const Util::Array<CallbackInfo>* cbArrays[] = {
        &this->onBeginFrameCallbacks,
        &this->onFrameCallbacks,
        &this->onEndFrameCallbacks,
        &this->onLoadCallbacks,
        &this->onSaveCallbacks,
    };

    for (auto arrPtr : cbArrays)
    {
        auto const& arr = *arrPtr;
        for (auto& cbinfo : arr)
        {
            if (MemDb::TableSignature::CheckBits(signature, GetInclusiveTableMask(cbinfo.filter)))
            {
                MemDb::TableSignature const& exclusive = GetExclusiveTableMask(cbinfo.filter);
                if (exclusive.IsValid())
                {
                    if (!MemDb::TableSignature::HasAny(signature, exclusive))
                        cbinfo.cache.Append(tid);
                }
                else
                {
                    cbinfo.cache.Append(tid);
                }
            }

        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::Prefilter()
{
    // this is just to compress the code a bit
    const Util::Array<CallbackInfo>* cbArrays[] = {
        &this->onBeginFrameCallbacks,
        &this->onFrameCallbacks,
        &this->onEndFrameCallbacks,
        &this->onLoadCallbacks,
        &this->onSaveCallbacks,
    };

    for (auto arrPtr : cbArrays)
    {
        auto const& arr = *arrPtr;
        for (auto& cbinfo : arr)
        {
            cbinfo.cache = this->db->Query(GetInclusiveTableMask(cbinfo.filter), GetExclusiveTableMask(cbinfo.filter));
        }
    }
}


//------------------------------------------------------------------------------
/**
*/
void
World::DefragmentCategoryInstances(CategoryId cat)
{
    Ptr<MemDb::Database> db = this->db;

    if (!db->IsValid(cat))
        return;

    MemDb::Table& table = db->GetTable(cat);
    MemDb::ColumnIndex ownerColumnId = db->GetColumnId(cat, GameServer::Singleton->state.ownerId);

    // defragment the table. Any instances that has been deleted will be swap'n'popped,
    // which means we need to update the entity mapping.
    // The move callback is signaled BEFORE the swap has happened.
    auto* const map = &(this->entityMap);
    SizeT numErased = db->Defragment(cat, [map, &ownerColumnId, &table](InstanceId from, InstanceId to)
    {
        Game::Entity fromEntity = ((Game::Entity*)(table.columns.Get<1>(ownerColumnId.id)))[from.id];
        Game::Entity toEntity = ((Game::Entity*)(table.columns.Get<1>(ownerColumnId.id)))[to.id];
        if (!IsValid(fromEntity))
        {
            // we need to add this instances new index to the to the freeids list, since it's been deleted.
            // the 'from' instance will be swapped with the 'to' instance, so we just add the 'to' id to the list;
            // and it will automatically be defragged
            table.freeIds.Append(to.id);
        }
        else
        {
            (*map)[fromEntity.index].instance = to;
            (*map)[toEntity.index].instance = from;
        }
    });
}

} // namespace Game
