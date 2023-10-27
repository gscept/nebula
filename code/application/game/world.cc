//------------------------------------------------------------------------------
//  @file entitypool.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "world.h"
#include "gameserver.h"
#include "api.h"
#include "category.h"
#include "util/queue.h"
#include "memdb/database.h"
#include "memdb/table.h"
#include "entitypool.h"
#include "memory/arenaallocator.h"
#include "ids/idallocator.h"
#include "basegamefeature/managers/blueprintmanager.h"
#include "imgui.h"
#include "game/componentinspection.h"
#include "basegamefeature/components/basegamefeature.h"

namespace Game
{

static Util::FixedArray<ComponentDecayBuffer> componentDecayTable;

//------------------------------------------------------------------------------
/**
*/
World::World(uint32_t hash)
    : numEntities(0),
      hash(hash)
{
    this->db = MemDb::Database::Create();
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
Game::Entity
World::CreateEntity()
{
    Entity const entity = this->AllocateEntity();
    return entity;
}

//------------------------------------------------------------------------------
/**
*/
Entity
World::AllocateEntity()
{
    Entity entity;
    if (this->pool.Allocate(entity))
    {
        this->entityMap.Append({MemDb::InvalidTableId, MemDb::InvalidRow});
    }
    this->numEntities++;
    return entity;
}

//------------------------------------------------------------------------------
/**
*/
void
World::DeallocateEntity(Entity entity)
{
    n_assert(!this->HasInstance(entity));
    this->pool.Deallocate(entity);
    this->numEntities--;
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
        &this->onActivateCallbacks,
    };

    auto const FillCache = [signature, tid](CallbackInfo& cbInfo)
    {
        if (MemDb::TableSignature::CheckBits(signature, GetInclusiveTableMask(cbInfo.filter)))
        {
            MemDb::TableSignature const& exclusive = GetExclusiveTableMask(cbInfo.filter);
            if (exclusive.IsValid())
            {
                if (!MemDb::TableSignature::HasAny(signature, exclusive))
                    cbInfo.cache.Append(tid);
            }
            else
            {
                cbInfo.cache.Append(tid);
            }
        }
    };

    for (auto arrPtr : cbArrays)
    {
        auto const& arr = *arrPtr;
        for (auto& cbInfo : arr)
        {
            FillCache(cbInfo);
        }
    }

    FillCache(activateAllInstancesCallback);
}

//------------------------------------------------------------------------------
/**
*/
void
World::Start()
{
    activateAllInstancesCallback.filter = Game::FilterBuilder().Including<Game::Owner>().Excluding<Game::IsActive>().Build();
    activateAllInstancesCallback.func = [](Game::World* world, Game::Dataset const& data)
    {
        // Move all partitions to their respective counterpart
        for (size_t i = 0; i < data.numViews; i++)
        {
            auto const& view = data.views[i];
            for (size_t instance = 0; instance < view.numInstances; instance++)
            {
                Entity const& entity = ((Game::Owner*)view.buffers[0])[instance].entity;
                world->AddComponent<Game::IsActive>(entity);
            }
        }
    };
}

//------------------------------------------------------------------------------
/**
*/
void
World::BeginFrame()
{
    int const numActiveCallBacks = this->onActivateCallbacks.Size();
    for (int i = 0; i < numActiveCallBacks; i++)
    {
        Dataset data = this->Query(this->onActivateCallbacks[i].filter, this->onActivateCallbacks[i].cache);
        this->onActivateCallbacks[i].func(this, data);
    }

    ExecuteAddComponentCommands();

    {
        // Move all newly created partitions to their respective table with IsActive flag included
        Dataset data = this->Query(this->activateAllInstancesCallback.filter, this->activateAllInstancesCallback.cache);
        this->activateAllInstancesCallback.func(this, data);
    }

    ExecuteAddComponentCommands();

    int const num = this->onBeginFrameCallbacks.Size();
    for (int i = 0; i < num; i++)
    {
        Dataset data = this->Query(this->onBeginFrameCallbacks[i].filter, this->onBeginFrameCallbacks[i].cache);
        this->onBeginFrameCallbacks[i].func(this, data);
    }

    ExecuteAddComponentCommands();
}

//------------------------------------------------------------------------------
/**
*/
void
World::SimFrame()
{
    int const num = this->onFrameCallbacks.Size();
    for (int i = 0; i < num; i++)
    {
        Dataset data = this->Query(this->onFrameCallbacks[i].filter, this->onFrameCallbacks[i].cache);
        this->onFrameCallbacks[i].func(this, data);
    }
    ExecuteAddComponentCommands();
}

//------------------------------------------------------------------------------
/**
*/
void
World::EndFrame()
{
    int const num = this->onEndFrameCallbacks.Size();
    for (int i = 0; i < num; i++)
    {
        Dataset data = this->Query(this->onEndFrameCallbacks[i].filter, this->onEndFrameCallbacks[i].cache);
        this->onEndFrameCallbacks[i].func(this, data);
    }

    // remove first, then add
    ExecuteRemoveComponentCommands();
    ExecuteAddComponentCommands();
}

//------------------------------------------------------------------------------
/**
*/
void
World::OnLoad()
{
    int num = this->onLoadCallbacks.Size();
    for (int i = 0; i < num; i++)
    {
        Dataset data = this->Query(this->onLoadCallbacks[i].filter, this->onLoadCallbacks[i].cache);
        this->onLoadCallbacks[i].func(this, data);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::OnSave()
{
    int num = this->onSaveCallbacks.Size();
    for (int i = 0; i < num; i++)
    {
        Dataset data = this->Query(this->onSaveCallbacks[i].filter, this->onSaveCallbacks[i].cache);
        this->onSaveCallbacks[i].func(this, data);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::PrefilterProcessors()
{
    // this is just to compress the code a bit
    const Util::Array<World::CallbackInfo>* cbArrays[] = {
        &this->onBeginFrameCallbacks,
        &this->onFrameCallbacks,
        &this->onEndFrameCallbacks,
        &this->onLoadCallbacks,
        &this->onSaveCallbacks,
        &this->onActivateCallbacks,
    };

    for (auto arrPtr : cbArrays)
    {
        auto const& arr = *arrPtr;
        for (auto& cbinfo : arr)
        {
            cbinfo.cache = this->db->Query(GetInclusiveTableMask(cbinfo.filter), GetExclusiveTableMask(cbinfo.filter));
        }
    }

    this->cacheValid = true;
}

//------------------------------------------------------------------------------
/**
*/
bool
World::Prefiltered() const
{
    return this->cacheValid;
}

//------------------------------------------------------------------------------
/**
*/
void
World::ManageEntities()
{
    // NOTE: The order of the following loops are important!

    // Clean up entities
    while (!this->deallocQueue.IsEmpty())
    {
        auto const cmd = this->deallocQueue.Dequeue();
        if (this->IsValid(cmd.entity))
        {
            MemDb::TableId const table = this->entityMap[cmd.entity.index].table;
            MemDb::RowId const row = this->entityMap[cmd.entity.index].instance;
            this->DeallocateInstance(table, row);
            this->entityMap[cmd.entity.index].table = MemDb::InvalidTableId;
            this->entityMap[cmd.entity.index].instance = MemDb::InvalidRow;
            this->DeallocateEntity(cmd.entity);
        }
    }

    // Allocate instances for new entities, reuse invalid instances if possible
    while (!this->allocQueue.IsEmpty())
    {
        auto const cmd = this->allocQueue.Dequeue();
        n_assert(this->IsValid(cmd.entity));
        this->AllocateInstance(cmd.entity, cmd.tid);
    }

    // Delete all remaining invalid instances
    Ptr<MemDb::Database> const& db = this->db;

    if (db.isvalid())
    {
        db->ForEachTable([this](MemDb::TableId tid) { this->Defragment(tid); });
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::Reset()
{
    this->db->Reset();
}

//------------------------------------------------------------------------------
/**
*/
Ptr<MemDb::Database>
World::GetDatabase()
{
    return this->db;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
World::CreateEntity(EntityCreateInfo const& info)
{
    World::AllocateInstanceCommand cmd;
    if (info.templateId != TemplateId::Invalid())
    {
        cmd.tid = info.templateId;
    }
    else
    {
        n_warning("Trying to instantiate an invalid template!");
        return Game::Entity::Invalid();
    }
    Entity const entity = this->AllocateEntity();
    cmd.entity = entity;

    if (!info.immediate)
    {
        this->allocQueue.Enqueue(std::move(cmd));
    }
    else
    {
        this->AllocateInstance(cmd.entity, cmd.tid);
    }

    return entity;
}

//------------------------------------------------------------------------------
/**
*/
void
World::DeleteEntity(Game::Entity entity)
{
    n_assert(this->IsValid(entity));
    
    if (this->HasInstance(entity))
    {
        World::DeallocInstanceCommand cmd;
        cmd.entity = entity;

        this->deallocQueue.Enqueue(std::move(cmd));
    }
    else
    {
        // entity hasn't been instantiated, can just delete the id straight away.
        this->DeallocateEntity(entity);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::DecayComponent(Game::ComponentId component, MemDb::TableId tableId, MemDb::ColumnIndex column, MemDb::RowId instance)
{
    if (MemDb::AttributeRegistry::Flags(component) & ComponentFlags::COMPONENTFLAG_DECAY)
    {
        if (component.id >= componentDecayTable.Size())
            componentDecayTable.Resize(
                component.id + 16
            ); // increment with a couple of extra elements, instead of doubling size, just to avoid extreme overallocation
        ComponentDecayBuffer& decayBuffer = componentDecayTable[component.id];

        uint64_t const typeSize = (uint64_t)MemDb::AttributeRegistry::TypeSize(component);

        if (decayBuffer.capacity == 0)
        {
            decayBuffer.size = 0;
            decayBuffer.capacity = 64;
            decayBuffer.buffer = Memory::Alloc(Memory::HeapType::DefaultHeap, decayBuffer.capacity * typeSize);
        }

        if (decayBuffer.capacity == decayBuffer.size)
        {
            void* oldBuffer = decayBuffer.buffer;
            decayBuffer.capacity *= 2;
            decayBuffer.buffer = Memory::Alloc(Memory::HeapType::DefaultHeap, decayBuffer.capacity * typeSize);
            Memory::Copy(oldBuffer, decayBuffer.buffer, typeSize * (uint64_t)decayBuffer.size);
            Memory::Free(Memory::HeapType::DefaultHeap, oldBuffer);
        }

        void* dst = ((byte*)decayBuffer.buffer) + (typeSize * (uint64_t)decayBuffer.size);
        decayBuffer.size++;
        Memory::Copy(this->db->GetTable(tableId).GetValuePointer(column, instance), dst, typeSize);
    }
}

//------------------------------------------------------------------------------
/**
*/
ComponentDecayBuffer const
World::GetDecayBuffer(Game::ComponentId component)
{
    if (component < componentDecayTable.Size())
        return componentDecayTable[component.id];
    else
        return ComponentDecayBuffer();
}

//------------------------------------------------------------------------------
/**
*/
void
World::ClearDecayBuffers()
{
    for (auto& cdb : componentDecayTable)
    {
        // TODO: shrink buffers if they're unreasonably big.
        cdb.size = 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
World::IsValid(Entity e)
{
    return this->pool.IsValid(e);
}

//------------------------------------------------------------------------------
/**
*/
bool
World::HasInstance(Entity e)
{
    n_assert(this->IsValid(e));
    return this->entityMap[e.index].instance != MemDb::InvalidRow;
}

//------------------------------------------------------------------------------
/**
*/
EntityMapping
World::GetEntityMapping(Game::Entity entity)
{
    n_assert(this->HasInstance(entity));
    return this->entityMap[entity.index];
}

//------------------------------------------------------------------------------
/**
   TODO: This is not thread safe!
*/
bool
World::HasComponent(Game::Entity const entity, ComponentId const component)
{
    EntityMapping mapping = this->GetEntityMapping(entity);
    return this->db->GetTable(mapping.table).HasAttribute(component);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
World::GetNumInstances(MemDb::TableId tid)
{
    return this->db->GetTable(tid).GetNumRows();
}

//------------------------------------------------------------------------------
/**
*/
void
World::ExecuteAddComponentCommands()
{
    auto sortFunc = [](const void* lhs, const void* rhs) -> int
    {
        Game::Entity arg1 = ((const AddStagedComponentCommand*)lhs)->entity;
        Game::Entity arg2 = ((const AddStagedComponentCommand*)rhs)->entity;
        return (arg1 > arg2) - (arg1 < arg2);
    };
    
    this->addStagedQueue.QuickSortWithFunc(sortFunc);

    SizeT i = 0;
    SizeT count = this->addStagedQueue.Size();

    auto* currentCmd = this->addStagedQueue.Begin();
    auto* end = this->addStagedQueue.End();
    while (currentCmd != end)
    {
        Entity currentEntity = currentCmd->entity;
        auto* firstCmdOfEntity = currentCmd;
        SizeT numEntityCmds = 0;
        while (currentCmd->entity == currentEntity && currentCmd != end)
        {
            numEntityCmds++;
            currentCmd++;
        }
        AddStagedComponentsToEntity(currentEntity, firstCmdOfEntity, numEntityCmds);
    }
    // release all memory of the staged components
    componentStageAllocator.Release();
    addStagedQueue.Reset();
}

//------------------------------------------------------------------------------
/**
*/
void
World::ExecuteRemoveComponentCommands()
{
    auto sortFunc = [](const void* lhs, const void* rhs) -> int
    {
        Game::Entity arg1 = ((const RemoveComponentCommand*)lhs)->entity;
        Game::Entity arg2 = ((const RemoveComponentCommand*)rhs)->entity;
        return (arg1 > arg2) - (arg1 < arg2);
    };

    this->removeComponentQueue.QuickSortWithFunc(sortFunc);

    SizeT i = 0;
    SizeT count = this->removeComponentQueue.Size();

    auto* currentCmd = this->removeComponentQueue.Begin();
    while (currentCmd != this->removeComponentQueue.End())
    {
        Entity currentEntity = currentCmd->entity;
        auto* firstCmdOfEntity = currentCmd;
        SizeT numEntityCmds = 0;
        while (currentCmd->entity == currentEntity)
        {
            numEntityCmds++;
            currentCmd++;
        }
        RemoveComponentsFromEntity(currentEntity, firstCmdOfEntity, numEntityCmds);
    }
    
    removeComponentQueue.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
World::AddStagedComponentsToEntity(Entity entity, AddStagedComponentCommand* cmds, SizeT numCmds)
{
    EntityMapping const mapping = this->GetEntityMapping(entity);
    MemDb::Table const& tbl = this->db->GetTable(mapping.table);
    MemDb::TableSignature signature = tbl.GetSignature();

    SizeT i;
    for (i = 0; i < numCmds; i++)
    {
        auto const* cmd = cmds + i;
        n_assert2(!signature.IsSet(cmd->componentId), "Tried to add a staged component to an entity that already has the given component!");
        signature.FlipBit(cmd->componentId);
    }
    
    MemDb::TableId newCategoryId = this->db->FindTable(signature);
    if (newCategoryId == MemDb::InvalidTableId)
    {
        CategoryCreateInfo info;
        auto const& cols = tbl.GetAttributes();
        info.components.SetSize(cols.Size() + numCmds);
        IndexT i;
        for (i = 0; i < cols.Size(); ++i)
        {
            info.components[i] = cols[i];
        }
        SizeT end = i + numCmds;
        IndexT cmdIndex = 0;
        for (; i < end; ++i, ++cmdIndex)
        {
            info.components[i] = cmds[cmdIndex].componentId;
        }
        
        newCategoryId = this->CreateEntityTable(info);
    }

    MemDb::RowId newInstance = this->Migrate(entity, newCategoryId);

    MemDb::Table& newTable = this->db->GetTable(newCategoryId);

    for (i = 0; i < numCmds; i++)
    {
        auto const* cmd = cmds + i;

        auto attrIndex = newTable.GetAttributeIndex(cmd->componentId);
        void* ptr = newTable.GetValuePointer(attrIndex, newInstance);
        Memory::Copy(cmd->data, ptr, cmd->dataSize);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::RemoveComponentsFromEntity(Entity entity, RemoveComponentCommand* cmds, SizeT numCmds)
{
    EntityMapping const mapping = this->GetEntityMapping(entity);
    MemDb::Table& tbl = this->db->GetTable(mapping.table);
    MemDb::TableSignature signature = this->db->GetTable(mapping.table).GetSignature();

    SizeT i;
    for (i = 0; i < numCmds; i++)
    {
        auto const* cmd = cmds + i;
#if NEBULA_DEBUG
        n_assert2(
            signature.IsSet(cmd->componentId),
            "Tried to remove a component from an entity that does not have the given component!"
        );
#endif
        signature.FlipBit(cmd->componentId);
    }

    MemDb::TableId newCategoryId = this->db->FindTable(signature);
    if (newCategoryId == MemDb::InvalidTableId)
    {
        CategoryCreateInfo info;
        auto const& cols = tbl.GetAttributes();
        info.components.SetSize(cols.Size() - numCmds);
        IndexT cIndex = 0;
        for (IndexT i = 0; i < cols.Size(); ++i)
        {
            IndexT k = 0;
            for (k = 0; k < numCmds; k++)
            {
                // check if the component should remain in the entity
                if (cols[i] == cmds[k].componentId)
                    break;
            }
            if (k == numCmds) // keep the component, otherwise discard it
                info.components[cIndex++] = cols[i];
        }

        newCategoryId = this->CreateEntityTable(info);
    }

    for (i = 0; i < numCmds; i++)
    {
        auto const* cmd = cmds + i;
        this->DecayComponent(cmd->componentId, mapping.table, tbl.GetAttributeIndex(cmd->componentId), mapping.instance);
    }

    this->Migrate(entity, newCategoryId);
}

//------------------------------------------------------------------------------
/**
*/
void*
World::GetInstanceBuffer(MemDb::TableId const tid, uint16_t partitionId, ComponentId const component)
{
    MemDb::Table& tbl = this->db->GetTable(tid);
    auto attrIndex = tbl.GetAttributeIndex(component);
#if NEBULA_DEBUG
    n_assert_fmt(
        attrIndex != MemDb::ColumnIndex::Invalid(),
        "GetInstanceBuffer: Entity table does not have component with id '%i'!\n",
        component.id
    );
#endif
    return tbl.GetBuffer(partitionId, attrIndex);
}

//------------------------------------------------------------------------------
/**
*/
MemDb::RowId
World::GetInstance(Entity entity)
{
    return this->GetEntityMapping(entity).instance;
}

//------------------------------------------------------------------------------
/**
*/
MemDb::TableId
World::CreateEntityTable(CategoryCreateInfo const& info)
{
    MemDb::TableSignature signature(info.components);

    MemDb::TableId categoryId = this->db->FindTable(signature);
    if (categoryId != MemDb::TableId::Invalid())
    {
        return categoryId;
    }

    constexpr ushort NUM_PROPS = 256;
    ComponentId components[NUM_PROPS];

    MemDb::TableCreateInfo tableInfo;
    tableInfo.name = info.name;
    tableInfo.numAttributes = 0;

    if (info.components[0] != GetComponentId<Game::Owner>() && info.components[1] != GetComponentId<Game::Transform>())
    {
        // always have owner and transform as first columns
        components[0] = GetComponentId<Owner>();
        components[1] = GetComponentId<Transform>();
        tableInfo.numAttributes = 2 + info.components.Size();

        n_assert(tableInfo.numAttributes < NUM_PROPS);

        for (int i = 0; i < info.components.Size(); i++)
        {
            components[i + 2] = info.components[i];
        }
        tableInfo.attributeIds = components;
    }
    else
    {
        tableInfo.numAttributes = info.components.Size();
        n_assert(tableInfo.numAttributes < NUM_PROPS);
        tableInfo.attributeIds = &info.components[0];
    }

    // Create an instance table
    categoryId = this->db->CreateTable(tableInfo);

    // "Prefilter" the processors with the new table (insert the table in the cache that accepts it)
    this->CacheTable(categoryId, this->db->GetTable(categoryId).GetSignature());

    return categoryId;
}



//------------------------------------------------------------------------------
/**
*/
MemDb::RowId
World::AllocateInstance(Entity entity, MemDb::TableId table)
{
    n_assert(this->pool.IsValid(entity));
    n_assert(this->entityMap[entity.index].instance == MemDb::InvalidRow);

    if (entity.index < this->entityMap.Size() && this->entityMap[entity.index].instance != MemDb::InvalidRow)
    {
        n_warning("Entity already registered!\n");
        return MemDb::InvalidRow;
    }

    MemDb::Table& tbl = this->db->GetTable(table);

    MemDb::RowId instance = tbl.AddRow();

    this->entityMap[entity.index] = {table, instance};

#if _DEBUG
    // make sure the first column in always owner
    n_assert(tbl.GetAttributeIndex(Game::GetComponentId<Game::Owner>()) == 0);
    n_assert(tbl.GetAttributeIndex(Game::GetComponentId<Game::Transform>()) == 1);
#endif

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)tbl.GetBuffer(instance.partition, 0);
    owners[instance.index] = entity;

    InitializeAllComponents(entity, table, instance);

    return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
World::InitializeAllComponents(Entity entity, MemDb::TableId tableId, MemDb::RowId row)
{
    MemDb::Table& tbl = this->db->GetTable(tableId);
    auto const& attributes = tbl.GetAttributes();
    for (IndexT i = 2; i < attributes.Size(); i++) // skip first two, since they're always owner and transform
    {
        MemDb::Attribute* attr = MemDb::AttributeRegistry::GetAttribute(attributes[i]);
        ComponentInterface* cInterface;
        cInterface = static_cast<ComponentInterface*>(attr);

        if (cInterface->Init != nullptr)
        {
            void* data = tbl.GetValuePointer(i, row);
            cInterface->Init(this, entity, data);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
MemDb::RowId
World::AllocateInstance(Entity entity, BlueprintId blueprint)
{
    n_assert(this->pool.IsValid(entity));
    n_assert(this->entityMap[entity.index].instance == MemDb::InvalidRow);

    if (entity.index < this->entityMap.Size() && this->entityMap[entity.index].instance != MemDb::InvalidRow)
    {
        n_warning("Entity already registered!\n");
        return MemDb::InvalidRow;
    }

    EntityMapping mapping = BlueprintManager::Instance()->Instantiate(this, blueprint);
    this->entityMap[entity.index] = mapping;

#if _DEBUG
    // make sure the first column in always owner
    n_assert(this->db->GetTable(mapping.table).GetAttributeIndex(GameServer::Singleton->state.ownerId) == 0);
    n_assert(this->db->GetTable(mapping.table).GetAttributeIndex(Game::GetComponentId<Game::Transform>()) == 1);
#endif

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)this->db->GetTable(mapping.table).GetBuffer(mapping.instance.partition, 0);
    owners[mapping.instance.index] = entity;

    InitializeAllComponents(entity, mapping.table, mapping.instance);

    return mapping.instance;
}

//------------------------------------------------------------------------------
/**
*/
MemDb::RowId
World::AllocateInstance(Entity entity, TemplateId templateId)
{
    n_assert(this->pool.IsValid(entity));
    n_assert(this->entityMap[entity.index].instance == MemDb::InvalidRow);

    if (entity.index < this->entityMap.Size() && this->entityMap[entity.index].instance != MemDb::InvalidRow)
    {
        n_warning("Entity instance already allocated!\n");
        return MemDb::InvalidRow;
    }

    EntityMapping mapping = BlueprintManager::Instance()->Instantiate(this, templateId);
    this->entityMap[entity.index] = mapping;

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)this->db->GetTable(mapping.table).GetBuffer(mapping.instance.partition, 0);
    owners[mapping.instance.index] = entity;

    InitializeAllComponents(entity, mapping.table, mapping.instance);

    return mapping.instance;
}

//------------------------------------------------------------------------------
/**
*/
void
World::DeallocateInstance(MemDb::TableId table, MemDb::RowId instance)
{
    n_assert(instance != MemDb::InvalidRow);

    // migrate managed properies to decay buffers so that we can allow the managers
    // to clean up any externally allocated resources.
    Util::Array<ComponentId> const& pids = this->db->GetTable(table).GetAttributes();
    const MemDb::ColumnIndex numColumns = pids.Size();
    for (MemDb::ColumnIndex column = 0; column < numColumns.id; column.id++)
    {
        Game::ComponentId component = pids[column.id];
        this->DecayComponent(component, table, column, instance);
    }

    this->db->GetTable(table).RemoveRow(instance);
}

//------------------------------------------------------------------------------
/**
*/
void
World::DeallocateInstance(Entity entity)
{
    MemDb::TableId& table = this->entityMap[entity.index].table;
    MemDb::RowId& instance = this->entityMap[entity.index].instance;

    this->DeallocateInstance(table, instance);

    table = MemDb::InvalidTableId;
    instance = MemDb::InvalidRow;
}

//------------------------------------------------------------------------------
/**
*/
MemDb::RowId
World::Migrate(Entity entity, MemDb::TableId newCategory)
{
    n_assert(this->HasInstance(entity));
    EntityMapping mapping = this->GetEntityMapping(entity);
    MemDb::RowId newInstance = MemDb::Table::MigrateInstance(
        this->db->GetTable(mapping.table), mapping.instance, this->db->GetTable(newCategory), false
    );

    // Defrag here to avoid entities existing in multiple tables
    this->Defragment(mapping.table);

    this->entityMap[entity.index] = {newCategory, newInstance};
    return newInstance;
}

//------------------------------------------------------------------------------
/**
    @param newInstances     Will be filled with the new instance ids in the destination table.
    @note   This assumes ALL entities in the entity array is of same table!
*/
void
World::Migrate(
    Util::Array<Entity> const& entities,
    MemDb::TableId fromCategory,
    MemDb::TableId newCategory,
    Util::FixedArray<MemDb::RowId>& newInstances
)
{
    if (newInstances.Size() != entities.Size())
    {
        newInstances.SetSize(entities.Size());
    }

    Util::Array<MemDb::RowId> instances;
    SizeT const num = entities.Size();
    instances.Reserve(num);

    for (auto entity : entities)
    {
        EntityMapping mapping = this->GetEntityMapping(entity);
#ifdef NEBULA_DEBUG
        n_assert(mapping.table == fromCategory);
#endif // NEBULA_DEBUG
        instances.Append(mapping.instance);
    }

    MemDb::Table::MigrateInstances(
        this->db->GetTable(fromCategory), instances, this->db->GetTable(newCategory), newInstances, false
    );

    // Defrag here to avoid entities existing in multiple tables
    this->Defragment(fromCategory);

    for (IndexT i = 0; i < num; i++)
    {
        this->entityMap[entities[i].index] = {newCategory, newInstances[i]};
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::RegisterProcessors(std::initializer_list<ProcessorHandle> handles)
{
    for (auto handle : handles)
    {
        ProcessorInfo const& info = Game::GameServer::Instance()->GetProcessorInfo(handle);

        // Setup frame callbacks
        if (info.OnBeginFrame != nullptr)
            this->onBeginFrameCallbacks.Append({handle, info.filter, info.OnBeginFrame});

        if (info.OnFrame != nullptr)
            this->onFrameCallbacks.Append({handle, info.filter, info.OnFrame});

        if (info.OnEndFrame != nullptr)
            this->onEndFrameCallbacks.Append({handle, info.filter, info.OnEndFrame});

        if (info.OnLoad != nullptr)
            this->onLoadCallbacks.Append({handle, info.filter, info.OnLoad});

        if (info.OnSave != nullptr)
            this->onSaveCallbacks.Append({handle, info.filter, info.OnSave});

        if (info.OnActivate != nullptr)
            this->onActivateCallbacks.Append({handle, info.filter, info.OnActivate});
    }

    this->cacheValid = false;
}

//------------------------------------------------------------------------------
/**
*/
void
World::Defragment(MemDb::TableId cat)
{
    if (!this->db->IsValid(cat))
        return;

    MemDb::Table& table = this->db->GetTable(cat);
    MemDb::ColumnIndex ownerColumnId = this->db->GetTable(cat).GetAttributeIndex(GetComponentId<Owner>());

    // defragment the table. Any instances that has been deleted will be swap'n'popped,
    // which means we need to update the entity mapping.
    // The move callback is signaled BEFORE the swap has happened.
    SizeT numErased = this->db->GetTable(cat).Defragment(
        [this, ownerColumnId](MemDb::Table::Partition* partition, MemDb::RowId from, MemDb::RowId to)
        {
            Game::Entity fromEntity = ((Game::Entity*)partition->columns[ownerColumnId.id])[from.index];
            Game::Entity toEntity = ((Game::Entity*)partition->columns[ownerColumnId.id])[to.index];
            if (!this->IsValid(fromEntity))
            {
                // we need to add these instances new index to the to the freeids list, since it's been deleted.
                // the 'from' instance will be swapped with the 'to' instance, so we just add the 'to' id to the list;
                // and it will automatically be defragged
                partition->freeIds.Append(to.index);
            }
            else if (this->entityMap[fromEntity.index].table == this->entityMap[toEntity.index].table)
            {
                // just swap the instances
                this->entityMap[fromEntity.index].instance = to;
                this->entityMap[toEntity.index].instance = from;
            }
            else
            {
                // if the entities does not belong to the same table, only update the
                // instance of the one that has been moved.
                // This is most likely due to an entity migration
                this->entityMap[fromEntity.index].instance = to;
            }
        }
    );
}



//------------------------------------------------------------------------------
/**
*/
void
World::SetComponentValue(World* world, Game::Entity entity, Game::ComponentId component, void* value, uint64_t size)
{
#if NEBULA_DEBUG
    n_assert2(
        size == MemDb::AttributeRegistry::TypeSize(component),
        "SetComponent: Provided value's type is not the correct size for the given ComponentId."
    );
#endif
    EntityMapping mapping = this->GetEntityMapping(entity);
    byte* const ptr = (byte*)this->GetInstanceBuffer(mapping.table, mapping.instance.partition, component);
    byte* valuePtr = ptr + (mapping.instance.index * size);
    Memory::Copy(value, valuePtr, size);
}

//------------------------------------------------------------------------------
/**
*/
void
World::RenderDebug()
{
    ImGui::Text("World Hash: %s", Util::FourCC(this->hash).AsString().AsCharPtr());
    ImGui::Separator();
    static bool showProcessors = true;
    ImGui::Checkbox("Show processors", &showProcessors);
    if (showProcessors)
    {
        ImGui::Text("Processors (?):");
        if (ImGui::IsItemHovered())
        {
            ImGui::SetTooltip("Processors are executed _after_ feature units for each event.");
        }

        auto PrintCallbackInfo = [](Game::World::CallbackInfo const& callback)
        {
            Game::ProcessorInfo const& info = Game::GameServer::Instance()->GetProcessorInfo(callback.handle);
            ImGui::Text(info.name.Value());
            ImGui::SameLine();
            ImGui::Text(" | Async: %s", info.async ? "true" : "false");
            ImGui::SameLine();
            ImGui::Text(" | Filter : %i", info.filter);
        };

        ImGui::TextDisabled("-- OnBeginFrame --");
        for (auto const& callback : this->onBeginFrameCallbacks)
            PrintCallbackInfo(callback);

        ImGui::TextDisabled("-- OnFrame --");
        for (auto const& callback : this->onFrameCallbacks)
            PrintCallbackInfo(callback);

        ImGui::TextDisabled("-- OnEndFrame --");
        for (auto const& callback : this->onEndFrameCallbacks)
            PrintCallbackInfo(callback);

        ImGui::TextDisabled("-- OnSave --");
        for (auto const& callback : this->onSaveCallbacks)
            PrintCallbackInfo(callback);

        ImGui::TextDisabled("-- OnLoad --");
        for (auto const& callback : this->onLoadCallbacks)
            PrintCallbackInfo(callback);

        ImGui::TextDisabled("-- OnActivate --");
        for (auto const& callback : this->onActivateCallbacks)
            PrintCallbackInfo(callback);

        ImGui::Separator();
    }

    static bool listInactive = false;
    ImGui::Checkbox("List inactive instances", &listInactive);
    ImGui::Text("Entity map:");
    ImGui::BeginChild(
        "ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar
    );
    {
        for (uint entityIndex = 0; entityIndex < this->entityMap.Size(); entityIndex++)
        {
            Game::EntityMapping entity = this->entityMap[entityIndex];

            if (!listInactive && (entity.table == MemDb::InvalidTableId || entity.instance == MemDb::InvalidRow))
                continue;

            ImGui::BeginGroup();
            ImGui::Text("[%i] ", entityIndex);
            ImGui::SameLine();
            ImGui::TextColored({1, 0.3f, 0, 1}, "tid:%i, row:%i", entity.table, entity.instance);
            if (entity.table != MemDb::TableId::Invalid())
            {
                ImGui::TextDisabled("- %s", this->db->GetTable(entity.table).name.Value());
                ImGui::EndGroup();
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::TextDisabled("- %s", this->db->GetTable(entity.table).name.Value());
                    MemDb::TableId const table = entity.table;
                    MemDb::RowId const row = entity.instance;

                    auto const& components = this->db->GetTable(table).GetAttributes();
                    for (auto component : components)
                    {
                        SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(component);
                        if (typeSize == 0)
                        {
                            // Type is flag type, just print the name, and then continue
                            ImGui::Text("_flag_: %s", MemDb::AttributeRegistry::GetAttribute(component)->name.Value());
                            ImGui::Separator();
                            continue;
                        }
                        void* data = this->GetInstanceBuffer(table, row.partition, component);
                        data = (byte*)data + (row.index * typeSize);
                        bool commitChange = false;
                        Game::ComponentInspection::DrawInspector(component, data, &commitChange);
                        ImGui::Separator();
                    }
                    ImGui::EndTooltip();
                }
            }
            else
            {
                ImGui::EndGroup();
                ImGui::TextDisabled("- ");
            }
            ImGui::Separator();
        }
    }
    ImGui::EndChild();
}

//------------------------------------------------------------------------------
/**
*/
void
World::Override(World* src, World* dst)
{
    dst->blueprintCatMap = src->blueprintCatMap;
    dst->entityMap = src->entityMap;
    dst->numEntities = src->numEntities;
    dst->pool = src->pool;
    dst->db = nullptr;
    dst->db = MemDb::Database::Create();
    src->db->Copy(dst->db);

    dst->PrefilterProcessors();
}


//------------------------------------------------------------------------------
/**
    @returns    Dataset with category table views.

    @note       The category table view buffer can be NULL if the filter contains
                a non-typed/flag component.
*/
Dataset
World::Query(Filter filter)
{
//#if NEBULA_ENABLE_PROFILING
//    //N_COUNTER_INCR("Calls to Game::Query", 1);
//    N_SCOPE_ACCUM(QueryTime, EntitySystem);
//#endif
    Util::Array<MemDb::TableId> tids = this->db->Query(GetInclusiveTableMask(filter), GetExclusiveTableMask(filter));

    return this->Query(filter, tids);
}

//------------------------------------------------------------------------------
/**
*/
Dataset
World::Query(Filter filter, Util::Array<MemDb::TableId>& tids)
{
    return Game::Query(this->db, tids, filter);
}

} // namespace Game
