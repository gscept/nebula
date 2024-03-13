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
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"

namespace Game
{

static Util::FixedArray<ComponentDecayBuffer> componentDecayTable;

//------------------------------------------------------------------------------
/**
*/
World::World(uint32_t hash)
    : numEntities(0),
      hash(hash),
      pipeline(this)
{
    this->db = MemDb::Database::Create();
    // clang-format off
    this->pipeline.RegisterFrameEvent( 10,   "OnBeginFrame");
    this->pipeline.RegisterFrameEvent( 100,  "OnFrame");
    this->pipeline.RegisterFrameEvent( 200,  "OnEndFrame");
    // clang-format on
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
*/
void
World::Start()
{
    this->pipeline.Begin();
}

//------------------------------------------------------------------------------
/**
*/
void
World::BeginFrame()
{
    this->pipeline.RunThru("OnBeginFrame");
    ExecuteAddComponentCommands();
}

//------------------------------------------------------------------------------
/**
*/
void
World::SimFrame()
{
    this->pipeline.RunThru("OnFrame");
    ExecuteAddComponentCommands();
}

//------------------------------------------------------------------------------
/**
*/
void
World::EndFrame()
{
    this->pipeline.RunThru("OnEndFrame");

    // remove first, then add
    ExecuteRemoveComponentCommands();
    ExecuteAddComponentCommands();

    this->pipeline.RunRemaining();
    this->pipeline.Reset();
}

//------------------------------------------------------------------------------
/**
*/
void
World::OnLoad()
{
    // TODO: Implement me
}

//------------------------------------------------------------------------------
/**
*/
void
World::OnSave()
{
    // TODO: Implement me
}

//------------------------------------------------------------------------------
/**
*/
void
World::PrefilterProcessors()
{
    this->pipeline.Prefilter(!this->cacheValid);
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
    this->cacheValid = false;
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
void
World::AddComponent(Entity entity, Game::ComponentId id)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(), "Adding component to entities while in an async processor is currently not supported!"
    );
#endif
    SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(id);
    void* data = this->componentStageAllocator.Alloc(typeSize);
    const void* defaultValue = MemDb::AttributeRegistry::DefaultValue(id);
    Memory::Copy(defaultValue, data, typeSize);

    AddStagedComponentCommand cmd = {
        .entity = entity,
        .componentId = id,
        .dataSize = typeSize,
        .data = data,
    };
    this->addStagedQueue.Append(cmd);

    if (this->componentInitializationEnabled)
    {
        MemDb::Attribute* attr = MemDb::AttributeRegistry::GetAttribute(id);
        ComponentInterface* cInterface;
        cInterface = static_cast<ComponentInterface*>(attr);

        if (cInterface->Init != nullptr)
        {
            // run initialization function if it exists.
            cInterface->Init(this, entity, data);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
World::RemoveComponent(Entity entity, ComponentId component)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Removing components from entities while executing an async processor is currently not supported!"
    );
#endif
    RemoveComponentCommand cmd = {
        .entity = entity,
        .componentId = component,
    };
    this->removeComponentQueue.Append(cmd);
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

    auto* currentCmd = this->removeComponentQueue.Begin();
    auto* end = this->removeComponentQueue.End();
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
        signature.SetBit(cmd->componentId);
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
        /*
        if (!signature.IsSet(cmd->componentId))
        {
            Util::String errorMsg = Util::String::Sprintf("Tried to remove component '%s' from an entity does not have the given component!", MemDb::AttributeRegistry::GetAttribute(cmd->componentId)->name.Value());
            n_error(errorMsg.AsCharPtr());
        }
        */
#endif
        signature.ClearBit(cmd->componentId);
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
void*
World::GetColumnData(MemDb::TableId const tid, uint16_t partitionId, MemDb::ColumnIndex const columnIndex)
{
    MemDb::Table& tbl = this->db->GetTable(tid);
    return tbl.GetBuffer(partitionId, columnIndex);
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
    MemDb::TableSignature const oldSignature(info.components);

    MemDb::TableId categoryId = this->db->FindTable(oldSignature);
    if (categoryId != MemDb::TableId::Invalid())
    {
        return categoryId;
    }

    constexpr ushort NUM_PROPS = 256;
    ComponentId components[NUM_PROPS];

    MemDb::TableCreateInfo tableInfo;
    tableInfo.name = info.name;
    tableInfo.numAttributes = 0;

    if (info.components[Game::Entity::Traits::fixed_column_index] != GetComponentId<Game::Entity>() &&
        info.components[Game::Position::Traits::fixed_column_index] != GetComponentId<Game::Position>() &&
        info.components[Game::Orientation::Traits::fixed_column_index] != GetComponentId<Game::Orientation>() &&
        info.components[Game::Scale::Traits::fixed_column_index] != GetComponentId<Game::Scale>()
        )
    {
        // always have owner and transform as first columns
        components[Game::Entity::Traits::fixed_column_index] = GetComponentId<Entity>();
        components[Game::Position::Traits::fixed_column_index] = GetComponentId<Position>();
        components[Game::Orientation::Traits::fixed_column_index] = GetComponentId<Orientation>();
        components[Game::Scale::Traits::fixed_column_index] = GetComponentId<Scale>();

        tableInfo.numAttributes = 4 + info.components.Size();

        n_assert(tableInfo.numAttributes < NUM_PROPS);

        for (int i = 0; i < info.components.Size(); i++)
        {
            components[i + 4] = info.components[i];
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
    this->pipeline.CacheTable(categoryId, this->db->GetTable(categoryId).GetSignature());
    
    return categoryId;
}

//------------------------------------------------------------------------------
/**
*/
MemDb::RowId
World::AllocateInstance(Entity entity, MemDb::TableId table, Util::Blob const* const data)
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
    n_assert(tbl.GetAttributeIndex(Game::GetComponentId<Game::Entity>()) == Game::Entity::Traits::fixed_column_index);
    n_assert(tbl.GetAttributeIndex(Game::GetComponentId<Game::Position>()) == Game::Position::Traits::fixed_column_index);
    n_assert(tbl.GetAttributeIndex(Game::GetComponentId<Game::Orientation>()) == Game::Orientation::Traits::fixed_column_index);
    n_assert(tbl.GetAttributeIndex(Game::GetComponentId<Game::Scale>()) == Game::Scale::Traits::fixed_column_index);
#endif

    if (data != nullptr)
    {
        tbl.DeserializeInstance(*data, instance);
    }

    // Set the owner of this instance.
    Game::Entity* owners = (Game::Entity*)tbl.GetBuffer(instance.partition, Game::Entity::Traits::fixed_column_index);
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
    if (!this->componentInitializationEnabled)
        return;

    MemDb::Table& tbl = this->db->GetTable(tableId);
    auto const& attributes = tbl.GetAttributes();
    for (IndexT i = 4; i < attributes.Size(); i++) // skip first four, since they're always owner and TRS
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
    n_assert(this->db->GetTable(mapping.table).GetAttributeIndex(Game::GetComponentId<Game::Entity>()) == Game::Entity::Traits::fixed_column_index);
    n_assert(this->db->GetTable(mapping.table).GetAttributeIndex(Game::GetComponentId<Game::Position>()) == Game::Position::Traits::fixed_column_index);
    n_assert(this->db->GetTable(mapping.table).GetAttributeIndex(Game::GetComponentId<Game::Orientation>()) == Game::Orientation::Traits::fixed_column_index);
    n_assert(this->db->GetTable(mapping.table).GetAttributeIndex(Game::GetComponentId<Game::Scale>()) == Game::Scale::Traits::fixed_column_index);
#endif

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)this->db->GetTable(mapping.table)
                               .GetBuffer(mapping.instance.partition, Game::Entity::Traits::fixed_column_index);
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
    Game::Entity* owners = (Game::Entity*)this->db->GetTable(mapping.table)
                               .GetBuffer(mapping.instance.partition, Game::Entity::Traits::fixed_column_index);
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
World::MoveInstance(MemDb::Table::Partition* partition, MemDb::RowId from, MemDb::RowId to)
{
    Game::Entity fromEntity = ((Game::Entity*)partition->columns[0])[from.index];
    Game::Entity toEntity = ((Game::Entity*)partition->columns[0])[to.index];
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

//------------------------------------------------------------------------------
/**
*/
void
World::Defragment(MemDb::TableId cat)
{
    if (!this->db->IsValid(cat))
        return;

#if NEBULA_DEBUG
    MemDb::ColumnIndex ownerColumnId = this->db->GetTable(cat).GetAttributeIndex(GetComponentId<Game::Entity>());
    n_assert(ownerColumnId == 0);
#endif
    // defragment the table. Any instances that has been deleted will be swap'n'popped,
    // which means we need to update the entity mapping.
    // The move callback is signaled BEFORE the swap has happened.
    UNUSED(SizeT) numErased = this->db->GetTable(cat).Defragment(
        [this](MemDb::Table::Partition* partition, MemDb::RowId from, MemDb::RowId to)
        {
            this->MoveInstance(partition, from, to);
        }
    );
}

//------------------------------------------------------------------------------
/**
*/
void
World::SetComponentValue(Game::Entity entity, Game::ComponentId component, void* value, uint64_t size)
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
    ImGui::Checkbox("Show Frame Pipeline", &showProcessors);
    if (showProcessors)
    {
        ImGui::Text("Pipeline (?):");
        if (ImGui::IsItemHovered())
        {
            //ImGui::SetTooltip("Processors are executed _after_ feature units for each event.");
            ImGui::SetTooltip("The pipeline that makes up the frame.\nThis consists of multiple frame events, possibly bundled in frame batches.");
        }

        auto const events = this->pipeline.GetFrameEvents();

        for (IndexT i = 0; i < events.Size(); i++)
        {
            ImGui::TextColored({0.8f, 0.4f, 0.8f, 1.0f}, "Event: %s", events[i]->name.Value());
            ImGui::SameLine();
            ImGui::Text(" | Order: %i", events[i]->order);

            auto const batches = events[i]->GetBatches();
            ImGui::Indent();
            for (IndexT b = 0; b < batches.Size(); b++)
            {
                auto const batch = batches[b];
                ImGui::TextColored({0.4f, 0.8f, 0.8f, 1.0f}, "Batch #%i", b);
                ImGui::SameLine();
                ImGui::Text(" | Order: %i", batch->order);
                ImGui::SameLine();
                ImGui::Text(" | Async: %s", batch->async ? "true" : "false");
                auto const processors = batch->GetProcessors();
                ImGui::Indent();
                for (IndexT p = 0; p < processors.Size(); p++)
                {
                    Game::Processor const* const processor = processors[p];
                    ImGui::TextColored({0.8f, 0.8f, 0.4f, 1.0f}, processor->name.AsCharPtr());
                    ImGui::SameLine();
                    ImGui::Text(" | Async: %s", processor->async ? "true" : "false");
                    ImGui::SameLine();
                    ImGui::TextColored({0.5,0.5,0.9f,1.0f}, " | Filter : %i", processor->filter);
                    if (ImGui::IsItemHovered())
                    {
                        ImGui::Indent();
                        auto const compsInc = Game::ComponentsInFilter(processor->filter);
                        if (compsInc.Size() > 0)
                        {
                            ImGui::TextColored({0.1f, 0.8f, 0.1f, 1.0f},"Includes entities with components:");
                            ImGui::BeginChild(1, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * compsInc.Size()));
                            {
                                for (auto c : compsInc)
                                {
                                    const char* componentName = MemDb::AttributeRegistry::GetAttribute(c)->name.Value();
                                    ImGui::Selectable(componentName);
                                }
                                ImGui::EndChild();
                            }
                        }
                        auto const compsEx = Game::ExcludedComponentsInFilter(processor->filter);
                        if (compsEx.Size() > 0)
                        {
                            ImGui::TextColored({0.8, 0.2, 0.2, 1.0f}, "Excludes entities with components:");
                            ImGui::BeginChild(2, ImVec2(0, ImGui::GetTextLineHeightWithSpacing() * compsEx.Size()));
                            {
                                for (auto c : compsEx)
                                {
                                    const char* componentName = MemDb::AttributeRegistry::GetAttribute(c)->name.Value();
                                    ImGui::Selectable(componentName);
                                }
                                ImGui::EndChild();
                            }
                        }
                        ImGui::Unindent();
                    }
                }
                ImGui::Unindent();
            }
            ImGui::Unindent();
        }
        /*
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
        */
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
            ImGui::TextColored({1, 0.3f, 0, 1}, "tid:%i, parition: %i, index: %i", entity.table, entity.instance.partition, entity.instance.index);
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

    if (src->componentInitializationEnabled == false && dst->componentInitializationEnabled)
    {
        // Initialize all component if the source db haven't already.
        Game::Filter filter = Game::FilterBuilder().Including<Game::Entity>().Build();
        Game::Dataset data = dst->Query(filter);

        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::View const& view = data.views[v];
            Game::Entity const* const entities = (Game::Entity*)view.buffers[0];

            for (uint16_t i = 0; i < view.numInstances; ++i)
            {
                Game::Entity const& entity = entities[i];
                MemDb::RowId row = {
                    .partition = view.partitionId,
                    .index = i
                };
                dst->InitializeAllComponents(entity, view.tableId, row);
            }
        }
        Game::DestroyFilter(filter);
    }

    dst->PrefilterProcessors();
}

//------------------------------------------------------------------------------
/**
*/
FramePipeline&
World::GetFramePipeline()
{
    return this->pipeline;
}

//------------------------------------------------------------------------------
/**
*/
void
World::MarkAsModified(Game::Entity entity)
{
    EntityMapping mapping = this->GetEntityMapping(entity);
    MemDb::Table& table = this->db->GetTable(mapping.table);
    MemDb::Table::Partition* partition = table.GetPartition(mapping.instance.partition);
    partition->modifiedRows.SetBit(mapping.instance.index);
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

//------------------------------------------------------------------------------
/**
*/
template <>
Game::Position
World::GetComponent(Entity entity)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Getting components from entities while executing an async processor is currently not supported!"
    );
#endif

    EntityMapping mapping = this->GetEntityMapping(entity);

    // This is a bit hacky, but we currently assume that position is always in column 1
    // This avoids a lookup that always evaluates to the same values anyways.
    MemDb::ColumnIndex const column = Game::Position::Traits::fixed_column_index;
    Game::Position* ptr = (Game::Position*)this->GetColumnData(mapping.table, mapping.instance.partition, column);
    return *(ptr + mapping.instance.index);
}

//------------------------------------------------------------------------------
/**
*/
template <>
Game::Orientation
World::GetComponent(Entity entity)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Getting components from entities while executing an async processor is currently not supported!"
    );
#endif

    EntityMapping mapping = this->GetEntityMapping(entity);

    // This is a bit hacky, but we currently assume that orientation is always in column 2
    // This avoids a lookup that always evaluates to the same values anyways.
    MemDb::ColumnIndex const column = Game::Orientation::Traits::fixed_column_index;
    Game::Orientation* ptr = (Game::Orientation*)this->GetColumnData(mapping.table, mapping.instance.partition, column);
    return *(ptr + mapping.instance.index);
}

//------------------------------------------------------------------------------
/**
*/
template <>
Game::Scale
World::GetComponent(Entity entity)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Getting components from entities while executing an async processor is currently not supported!"
    );
#endif

    EntityMapping mapping = this->GetEntityMapping(entity);

    // This is a bit hacky, but we currently assume that scale is always in column 3
    // This avoids a lookup that always evaluates to the same values anyways.
    MemDb::ColumnIndex const column = Game::Scale::Traits::fixed_column_index;
    Game::Scale* ptr = (Game::Scale*)this->GetColumnData(mapping.table, mapping.instance.partition, column);
    return *(ptr + mapping.instance.index);
}

//------------------------------------------------------------------------------
/**
*/
template <>
void
World::SetComponent(Entity entity, Game::Position const& value)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Settings components in arbitrary entities while executing an async processor is currently not supported!"
    );
#endif

    EntityMapping mapping = this->GetEntityMapping(entity);

    // This is a bit hacky, but we currently assume that position is always in column 1
    // This avoids a lookup that always evaluates to the same values anyways.
    MemDb::ColumnIndex const column = Game::Position::Traits::fixed_column_index;
    Game::Position* ptr = (Game::Position*)this->GetColumnData(mapping.table, mapping.instance.partition, column);
    *(ptr + mapping.instance.index) = value;
}

//------------------------------------------------------------------------------§
/**
*/
template <>
void
World::SetComponent(Entity entity, Game::Orientation const& value)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Settings components in arbitrary entities while executing an async processor is currently not supported!"
    );
#endif

    EntityMapping mapping = this->GetEntityMapping(entity);

    // This is a bit hacky, but we currently assume that orientation is always in column 2
    // This avoids a lookup that always evaluates to the same values anyways.
    MemDb::ColumnIndex const column = Game::Orientation::Traits::fixed_column_index;
    Game::Orientation* ptr = (Game::Orientation*)this->GetColumnData(mapping.table, mapping.instance.partition, column);
    *(ptr + mapping.instance.index) = value;
}

//------------------------------------------------------------------------------
/**
*/
template <>
void
World::SetComponent(Entity entity, Game::Scale const& value)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Settings components in arbitrary entities while executing an async processor is currently not supported!"
    );
#endif

    EntityMapping mapping = this->GetEntityMapping(entity);

    // This is a bit hacky, but we currently assume that scale is always in column 3
    // This avoids a lookup that always evaluates to the same values anyways.
    MemDb::ColumnIndex const column = Game::Scale::Traits::fixed_column_index;
    Game::Scale* ptr = (Game::Scale*)this->GetColumnData(mapping.table, mapping.instance.partition, column);
    *(ptr + mapping.instance.index) = value;
}

} // namespace Game
