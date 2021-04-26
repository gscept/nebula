//------------------------------------------------------------------------------
//  api.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "api.h"
#include "gameserver.h"
#include "ids/idallocator.h"
#include "memdb/tablesignature.h"
#include "memdb/database.h"
#include "memory/arenaallocator.h"
#include "basegamefeature/managers/blueprintmanager.h"
#include "profiling/profiling.h"
#include "util/fixedarray.h"

namespace Game
{

//------------------------------------------------------------------------------
using InclusiveTableMask = MemDb::TableSignature;
using ExclusiveTableMask = MemDb::TableSignature;
using PropertyArray = Util::FixedArray<PropertyId>;
using AccessModeArray = Util::FixedArray<AccessMode>;

Ids::IdAllocator<InclusiveTableMask, ExclusiveTableMask, PropertyArray, AccessModeArray>  filterAllocator;
static Memory::ArenaAllocator<sizeof(Dataset::CategoryTableView) * 256> viewAllocator;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
using RegPidQueue = Util::Queue<Op::RegisterProperty>;
using DeregPidQueue = Util::Queue<Op::DeregisterProperty>;

static Ids::IdAllocator<World*, RegPidQueue, DeregPidQueue> opBufferAllocator;
static Memory::ArenaAllocator<1024> opAllocator;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
Util::FixedArray<PropertyDecayBuffer> propertyDecayTable;
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
*/
World*
GetWorld(uint32_t hash)
{
    return GameServer::Instance()->GetWorld(hash);
}

//------------------------------------------------------------------------------
/**
*/
World*
CreateWorld(WorldCreateInfo const& info)
{
    return GameServer::Instance()->CreateWorld(info);
}

//------------------------------------------------------------------------------
/**
*/
Ptr<MemDb::Database>
GetWorldDatabase(World* world)
{
    n_assert(GameServer::HasInstance());
    return world->db;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
CreateEntity(World* world, EntityCreateInfo const& info)
{
    n_assert(GameServer::HasInstance());
    GameServer::State* const state = &GameServer::Singleton->state;

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
    Entity const entity = AllocateEntity(world);
    cmd.entity = entity;

    if (!info.immediate)
    {
        world->allocQueue.Enqueue(std::move(cmd));
    }
    else
    {
        AllocateInstance(world, cmd.entity, cmd.tid);
    }

    return entity;
}

//------------------------------------------------------------------------------
/**
*/
void
DeleteEntity(World* world, Game::Entity entity)
{
    n_assert(IsValid(world, entity));
    n_assert(GameServer::HasInstance());
    
    if (IsActive(world, entity))
    {
        World::DeallocInstanceCommand cmd;
        cmd.entity = entity;
        
        world->deallocQueue.Enqueue(std::move(cmd));
    }
    else
    {
        // entity hasn't been instantiated, can just delete the id straight away.
        DeallocateEntity(world, entity);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DecayProperty(Game::World* world, Game::PropertyId pid, MemDb::TableId tableId, MemDb::ColumnIndex column, MemDb::Row instance)
{
    if (MemDb::TypeRegistry::Flags(pid) & PropertyFlags::PROPERTYFLAG_MANAGED)
    {
        PropertyDecayBuffer& pdb = propertyDecayTable[pid.id];
        SizeT const typeSize = MemDb::TypeRegistry::TypeSize(pid);

        if (pdb.capacity == pdb.size)
        {
            void* oldBuffer = pdb.buffer;
            pdb.capacity *= 2;
            pdb.buffer = Memory::Alloc(Memory::HeapType::AppHeap, pdb.capacity);
            Memory::Copy(oldBuffer, pdb.buffer, typeSize * pdb.size);
            Memory::Free(Memory::HeapType::AppHeap, oldBuffer);
        }

        void* dst = ((byte*)pdb.buffer) + (typeSize * pdb.size);
        pdb.size++;
        Memory::Copy(world->db->GetValuePointer(tableId, column, instance), dst, typeSize);
    }
}

//------------------------------------------------------------------------------
/**
    @todo   There should be better and more clean ways of doing this.
            It's ugly and inefficient...
*/
OpBuffer
CreateOpBuffer(World* world)
{
    OpBuffer id = opBufferAllocator.Alloc();
    opBufferAllocator.Get<0>(id) = world;
    return id;
}

//------------------------------------------------------------------------------
/**
    @todo   We can bundle all add and remove property for each entity into one
            migration.
            We can also batch them based on their new category, so we won't
            need to do as many column id lookups.
    @todo   This is not thread safe. Either, we keep it like it is and make sure
            this function is always called synchronously, or add a critical section?
*/
void
Dispatch(OpBuffer& buffer)
{
    World* world = opBufferAllocator.Get<0>(buffer);
    RegPidQueue& registerPropertyQueue = opBufferAllocator.Get<1>(buffer);
    DeregPidQueue& deregisterPropertyQueue = opBufferAllocator.Get<2>(buffer);

    while (!registerPropertyQueue.IsEmpty())
    {
        auto op = registerPropertyQueue.Dequeue();
        Game::Execute(world, op);
    }

    while (!deregisterPropertyQueue.IsEmpty())
    {
        auto op = deregisterPropertyQueue.Dequeue();
        Game::Execute(world, op);
    }

    opBufferAllocator.Dealloc(buffer);
    buffer = InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    @todo   optimize
*/
void
AddOp(OpBuffer buffer, Op::RegisterProperty op)
{
    if (op.value != nullptr)
    {
        SizeT const typeSize = MemDb::TypeRegistry::TypeSize(op.pid);
        void* value = opAllocator.Alloc(typeSize);
        Memory::Copy(op.value, value, typeSize);
        op.value = value;
    }
    opBufferAllocator.Get<1>(buffer).Enqueue(op);
}

//------------------------------------------------------------------------------
/**
*/
void
AddOp(OpBuffer buffer, Op::DeregisterProperty const& op)
{
    opBufferAllocator.Get<2>(buffer).Enqueue(op);
}

//------------------------------------------------------------------------------
/**
    @todo   Optimize me
*/
void
Execute(World* world, Op::RegisterProperty const& op)
{
    EntityMapping const mapping = GetEntityMapping(world, op.entity);
    
    MemDb::TableSignature signature = world->db->GetTableSignature(mapping.category);
    if (signature.IsSet(op.pid))
        return;

    signature.FlipBit(op.pid);

    MemDb::TableId newCategoryId = world->db->FindTable(signature);
    if (newCategoryId == MemDb::InvalidTableId)
    {
        CategoryCreateInfo info;
        auto const& cols = world->db->GetTable(mapping.category).properties;
        info.properties.SetSize(cols.Size() + 1);
        IndexT i;
        for (i = 0; i < cols.Size(); ++i)
        {
            info.properties[i] = cols[i];
        }
        info.properties[i] = op.pid;

        newCategoryId = CreateEntityTable(world, info);
    }

    MemDb::Row newInstance = Migrate(world, op.entity, newCategoryId);

    if (op.value == nullptr)
        return; // default value should already be set

    auto cid = world->db->GetColumnId(newCategoryId, op.pid);
    void* ptr = world->db->GetValuePointer(newCategoryId, cid, newInstance);
    Memory::Copy(op.value, ptr, MemDb::TypeRegistry::TypeSize(op.pid));
}

//------------------------------------------------------------------------------
/**
    @bug   If you deregister a managed property, the property will just disappear
           without letting the manager clean up any resources, leading to memleaks.

            Possible fix: We can change the "decay system" to have a decay buffer
            for each property that is managed. That way, when an entity is deleted,
            or a property is deregistered, we copy the managed properties into their
            respective decay buffer. We can create a separate frame event for decay
            handling as well. This means each table won't have their own decay tables,
            but each property will have a decay buffer instead that is not a fully fledged
            table. The buffers won't care about which entities owned the instances.
            The decay buffers should be cleaned at the specific decay cleaup event each
            frame.
*/
void
Execute(World* world, Op::DeregisterProperty const& op)
{
#if NEBULA_DEBUG
    n_assert(Game::HasProperty(world, op.entity, op.pid));
#endif
    EntityMapping const mapping = GetEntityMapping(world, op.entity);
    MemDb::TableSignature signature = world->db->GetTableSignature(mapping.category);
    if (!signature.IsSet(op.pid))
        return;

    signature.FlipBit(op.pid);

    MemDb::TableId newCategoryId = world->db->FindTable(signature);
    if (newCategoryId == MemDb::InvalidTableId)
    {
        CategoryCreateInfo info;
        auto const& cols = world->db->GetTable(mapping.category).properties;
        SizeT const num = cols.Size();
        info.properties.SetSize(num - 1);
        int col = 0;
        for (int i = 0; i < num; ++i)
        {
            if (cols[i] == op.pid)
                continue;

            info.properties[col++] = cols[i];
        }
        
        newCategoryId = CreateEntityTable(world, info);
    }

    DecayProperty(world, op.pid, mapping.category, world->db->GetColumnId(mapping.category, op.pid), mapping.instance);

    Migrate(world, op.entity, newCategoryId);
}

//------------------------------------------------------------------------------
/**
*/
void
ReleaseAllOps()
{
    opAllocator.Release();
}

//------------------------------------------------------------------------------
/**
*/
Filter
CreateFilter(FilterCreateInfo const& info)
{
    n_assert(info.numInclusive > 0);
    uint32_t filter = filterAllocator.Alloc();

    PropertyArray inclusiveArray;
    inclusiveArray.Resize(info.numInclusive);
    for (uint8_t i = 0; i < info.numInclusive; i++)
    {
        inclusiveArray[i] = info.inclusive[i];
    }

    PropertyArray exclusiveArray;
    exclusiveArray.Resize(info.numExclusive);
    for (uint8_t i = 0; i < info.numExclusive; i++)
    {
        exclusiveArray[i] = info.exclusive[i];
    }

    AccessModeArray accessArray;
    accessArray.Resize(info.numInclusive);
    for (uint8_t i = 0; i < info.numInclusive; i++)
    {
        accessArray[i] = info.access[i];
    }

    filterAllocator.Set(filter,
        InclusiveTableMask(inclusiveArray),
        ExclusiveTableMask(exclusiveArray),
        inclusiveArray,
        accessArray
    );

    return filter;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyFilter(Filter filter)
{
    filterAllocator.Dealloc(filter);
}

//------------------------------------------------------------------------------
/**
*/
ProcessorHandle
CreateProcessor(ProcessorCreateInfo const& info)
{
    return Game::GameServer::Instance()->CreateProcessor(info);
}

//------------------------------------------------------------------------------
/**
*/
void
ReleaseDatasets()
{
    viewAllocator.Release();
}

//------------------------------------------------------------------------------
/**
    @returns    Dataset with category table views.

    @note       The category table view buffer can be NULL if the filter contains
                a non-typed/flag property.
*/
Dataset Query(World* world, Filter filter)
{
#if NEBULA_ENABLE_PROFILING
    //N_COUNTER_INCR("Calls to Game::Query", 1);
    N_SCOPE_ACCUM(QueryTime, EntitySystem);
#endif
    Ptr<MemDb::Database> db = Game::GetWorldDatabase(world);

    Util::Array<MemDb::TableId> tids = db->Query(filterAllocator.Get<0>(filter), filterAllocator.Get<1>(filter));

    return Query(world, tids, filter);
}

//------------------------------------------------------------------------------
/**
*/
Dataset
Query(World* world, Util::Array<MemDb::TableId>& tids, Filter filter)
{
    Ptr<MemDb::Database> db = Game::GetWorldDatabase(world);
    return Query(db, tids, filter);
}

//------------------------------------------------------------------------------
/**
*/
Dataset
Query(Ptr<MemDb::Database> const& db, Util::Array<MemDb::TableId>& tids, Filter filter)
{
    Dataset data;
    data.numViews = 0;

    if (tids.Size() == 0)
    {
        data.views = nullptr;
        return data;
    }

    data.views = (Dataset::CategoryTableView*)viewAllocator.Alloc(sizeof(Dataset::CategoryTableView) * tids.Size());

    PropertyArray const& properties = filterAllocator.Get<2>(filter);

    for (IndexT tableIndex = 0; tableIndex < tids.Size(); tableIndex++)
    {
        if (db->IsValid(tids[tableIndex]))
        {
            SizeT const numRows = db->GetNumRows(tids[tableIndex]);
            if (numRows > 0)
            {
                Dataset::CategoryTableView* view = data.views + data.numViews;
                view->cid = tids[tableIndex];

                MemDb::Table const& tbl = db->GetTable(tids[tableIndex]);

                IndexT i = 0;
                for (auto pid : properties)
                {
                    MemDb::ColumnIndex colId = db->GetColumnId(tbl.tid, pid);
                    // Check if the property is a flag, and return a nullptr in that case.
                    if (colId != InvalidIndex)
                        view->buffers[i] = db->GetBuffer(tbl.tid, colId);
                    else
                        view->buffers[i] = nullptr;
                    i++;
                }

                view->numInstances = numRows;
                data.numViews++;
            }
        }
        else
        {
            tids.EraseIndexSwap(tableIndex);
            // re-run the same index
            tableIndex--;
        }
    }

    return data;
}

//------------------------------------------------------------------------------
/**
*/
bool
IsValid(World* world, Entity e)
{
    return world->pool.IsValid(e);
}

//------------------------------------------------------------------------------
/**
*/
bool
IsActive(World* world, Entity e)
{
    n_assert(IsValid(world, e));
    return world->entityMap[e.index].instance != MemDb::InvalidRow;
}

//------------------------------------------------------------------------------
/**
*/
EntityMapping
GetEntityMapping(World* world, Game::Entity entity)
{
    n_assert(IsActive(world, entity));
    return world->entityMap[entity.index];
}

//------------------------------------------------------------------------------
/**
*/
PropertyId
CreateProperty(PropertyCreateInfo const& info)
{
    PropertyId const pid = MemDb::TypeRegistry::Register(info.name, info.byteSize, info.defaultValue, info.flags);
    if (info.flags & PropertyFlags::PROPERTYFLAG_MANAGED)
    {
        if (pid.id >= propertyDecayTable.Size())
            propertyDecayTable.Resize(pid.id + 16); // increment with a couple of extra elements, instead of doubling size, just to avoid extreme overallocation
        PropertyDecayBuffer& pdb = propertyDecayTable[pid.id];
        pdb.size = 0;
        pdb.capacity = 64;
        pdb.buffer = Memory::Alloc(Memory::HeapType::AppHeap, pdb.capacity);
    }
    return pid;
}

//------------------------------------------------------------------------------
/**
*/
PropertyId
GetPropertyId(Util::StringAtom name)
{
    return MemDb::TypeRegistry::GetPropertyId(name);
}

//------------------------------------------------------------------------------
/**
   TODO: This is not thread safe!
*/
bool
HasProperty(World* world, Game::Entity const entity, PropertyId const pid)
{
    EntityMapping mapping = GetEntityMapping(world, entity);
    return world->db->HasProperty(mapping.category, pid);
}

//------------------------------------------------------------------------------
/**
*/
BlueprintId
GetBlueprintId(Util::StringAtom name)
{
    return BlueprintManager::GetBlueprintId(name);
}

//------------------------------------------------------------------------------
/**
*/
TemplateId
GetTemplateId(Util::StringAtom name)
{
    return BlueprintManager::GetTemplateId(name);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
GetNumInstances(World* world, MemDb::TableId tid)
{
    return world->db->GetNumRows(tid);
}

//------------------------------------------------------------------------------
/**
*/
void*
GetInstanceBuffer(World* world, MemDb::TableId const tid, PropertyId const pid)
{
    Ptr<MemDb::Database> db = world->db;
    auto cid = db->GetColumnId(tid, pid);
#if NEBULA_DEBUG
    n_assert_fmt(cid != MemDb::ColumnIndex::Invalid(), "GetInstanceBuffer: Category does not have property with id '%i'!\n", pid.id);
#endif
    return db->GetBuffer(tid, cid);
}

//------------------------------------------------------------------------------
/**
*/
InclusiveTableMask const&
GetInclusiveTableMask(Filter filter)
{
    return filterAllocator.Get<0>(filter);
}

//------------------------------------------------------------------------------
/**
*/
ExclusiveTableMask const&
GetExclusiveTableMask(Filter filter)
{
    return filterAllocator.Get<1>(filter);
}

//------------------------------------------------------------------------------
/**
*/
MemDb::Row
GetInstance(World* world, Entity entity)
{
    return GetEntityMapping(world, entity).instance;
}


//------------------------------------------------------------------------------
/**
*/
MemDb::TableId
CreateEntityTable(World* world, CategoryCreateInfo const& info)
{
    MemDb::TableSignature signature(info.properties);

    MemDb::TableId categoryId = world->db->FindTable(signature);
    if (categoryId != MemDb::TableId::Invalid())
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
    categoryId = world->db->CreateTable(tableInfo);

    // "Prefilter" the processors with the new table (insert the table in the cache that accepts it)
    world->CacheTable(categoryId, world->db->GetTableSignature(categoryId));

    return categoryId;
}

//------------------------------------------------------------------------------
/**
*/
MemDb::Row
AllocateInstance(World* world, Entity entity, MemDb::TableId category)
{
    n_assert(world->pool.IsValid(entity));
    n_assert(world->entityMap[entity.index].instance == MemDb::InvalidRow);

    if (entity.index < world->entityMap.Size() && world->entityMap[entity.index].instance != MemDb::InvalidRow)
    {
        n_warning("Entity already registered!\n");
        return InvalidIndex;
    }

    MemDb::Row instance = world->db->AllocateRow(category);

    world->entityMap[entity.index] = { category, instance };

    // Just make sure the first column in always owner!
    n_assert(world->db->GetColumnId(category, GameServer::Singleton->state.ownerId) == 0);

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)world->db->GetBuffer(category, 0);
    owners[instance] = entity;

    return instance;
}

//------------------------------------------------------------------------------
/**
*/
MemDb::Row
AllocateInstance(World* world, Entity entity, BlueprintId blueprint)
{
    n_assert(world->pool.IsValid(entity));
    n_assert(world->entityMap[entity.index].instance == MemDb::InvalidRow);

    if (entity.index < world->entityMap.Size() && world->entityMap[entity.index].instance != MemDb::InvalidRow)
    {
        n_warning("Entity already registered!\n");
        return InvalidIndex;
    }

    EntityMapping mapping = BlueprintManager::Instance()->Instantiate(world, blueprint);
    world->entityMap[entity.index] = mapping;

    // Just make sure the first column in always owner!
    n_assert(world->db->GetColumnId(mapping.category, GameServer::Singleton->state.ownerId) == 0);

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)world->db->GetBuffer(mapping.category, 0);
    owners[mapping.instance] = entity;

    return mapping.instance;
}

//------------------------------------------------------------------------------
/**
*/
MemDb::Row
AllocateInstance(World* world, Entity entity, TemplateId templateId)
{
    n_assert(world->pool.IsValid(entity));
    n_assert(world->entityMap[entity.index].instance == MemDb::InvalidRow);

    if (entity.index < world->entityMap.Size() && world->entityMap[entity.index].instance != MemDb::InvalidRow)
    {
        n_warning("Entity instance already allocated!\n");
        return InvalidIndex;
    }

    EntityMapping mapping = BlueprintManager::Instance()->Instantiate(world, templateId);
    world->entityMap[entity.index] = mapping;

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)world->db->GetBuffer(mapping.category, 0);
    owners[mapping.instance] = entity;

    return mapping.instance;
}

//------------------------------------------------------------------------------
/**
*/
void
DeallocateInstance(World* world, MemDb::TableId category, MemDb::Row instance)
{
    n_assert(instance != MemDb::InvalidRow);

    // migrate managed properies to decay buffers so that we can allow the managers
    // to clean up any externally allocated resources.
    Util::Array<PropertyId> const& pids = world->db->GetTable(category).properties;
    const MemDb::ColumnIndex numColumns = pids.Size();
    for (MemDb::ColumnIndex column = 0; column < numColumns.id; column.id++)
    {
        Game::PropertyId pid = pids[column.id];
        DecayProperty(world, pid, category, column, instance);
    }

    world->db->DeallocateRow(category, instance);
}

//------------------------------------------------------------------------------
/**
*/
void
DeallocateInstance(World* world, Entity entity)
{
    MemDb::TableId& category = world->entityMap[entity.index].category;
    MemDb::Row& instance = world->entityMap[entity.index].instance;

    DeallocateInstance(world, category, instance);

    category = MemDb::InvalidTableId;
    instance = MemDb::InvalidRow;
}

//------------------------------------------------------------------------------
/**
*/
MemDb::Row
Migrate(World* world, Entity entity, MemDb::TableId newCategory)
{
    n_assert(IsActive(world, entity));
    EntityMapping mapping = GetEntityMapping(world, entity);
    MemDb::Row newInstance = world->db->MigrateInstance(mapping.category, mapping.instance, newCategory, false);
    Defragment(world, mapping.category);
    world->entityMap[entity.index] = { newCategory, newInstance };
    return newInstance;
}

//------------------------------------------------------------------------------
/**
    @param newInstances     Will be filled with the new instance ids in the destination category.
    @note   This assumes ALL entities in the entity array is of same category!
*/
void
Migrate(World* world, Util::Array<Entity> const& entities, MemDb::TableId fromCategory, MemDb::TableId newCategory, Util::FixedArray<IndexT>& newInstances)
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
        EntityMapping mapping = GetEntityMapping(world, entity);
#ifdef NEBULA_DEBUG
        n_assert(mapping.category == fromCategory);
#endif // NEBULA_DEBUG
        instances.Append(mapping.instance);
    }

    world->db->MigrateInstances(fromCategory, instances, newCategory, newInstances, false);
    Defragment(world, fromCategory);
    for (IndexT i = 0; i < num; i++)
    {
        world->entityMap[entities[i].index] = { newCategory, (MemDb::Row)newInstances[i] };
    }
}

//------------------------------------------------------------------------------
/**
*/
void
RegisterProcessors(World* world, std::initializer_list<ProcessorHandle> handles)
{
    for (auto handle : handles)
    {
        ProcessorInfo const& info = Game::GameServer::Instance()->GetProcessorInfo(handle);

        // Setup frame callbacks
        if (info.OnBeginFrame != nullptr)
            world->onBeginFrameCallbacks.Append({ handle, info.filter, info.OnBeginFrame });

        if (info.OnFrame != nullptr)
            world->onFrameCallbacks.Append({ handle, info.filter, info.OnFrame });

        if (info.OnEndFrame != nullptr)
            world->onEndFrameCallbacks.Append({ handle, info.filter, info.OnEndFrame });

        if (info.OnLoad != nullptr)
            world->onLoadCallbacks.Append({ handle, info.filter, info.OnLoad });

        if (info.OnSave != nullptr)
            world->onSaveCallbacks.Append({ handle, info.filter, info.OnSave });
    }
}

//------------------------------------------------------------------------------
/**
*/
void
PrefilterProcessors(World* world)
{
    // this is just to compress the code a bit
    const Util::Array<World::CallbackInfo>* cbArrays[] = {
        &world->onBeginFrameCallbacks,
        &world->onFrameCallbacks,
        &world->onEndFrameCallbacks,
        &world->onLoadCallbacks,
        &world->onSaveCallbacks,
    };

    for (auto arrPtr : cbArrays)
    {
        auto const& arr = *arrPtr;
        for (auto& cbinfo : arr)
        {
            cbinfo.cache = world->db->Query(GetInclusiveTableMask(cbinfo.filter), GetExclusiveTableMask(cbinfo.filter));
        }
    }
}


//------------------------------------------------------------------------------
/**
*/
void
Defragment(World* world, MemDb::TableId cat)
{
    Ptr<MemDb::Database> db = world->db;

    if (!db->IsValid(cat))
        return;

    MemDb::Table& table = db->GetTable(cat);
    MemDb::ColumnIndex ownerColumnId = db->GetColumnId(cat, GameServer::Singleton->state.ownerId);

    // defragment the table. Any instances that has been deleted will be swap'n'popped,
    // which means we need to update the entity mapping.
    // The move callback is signaled BEFORE the swap has happened.
    SizeT numErased = db->Defragment(cat, [world, ownerColumnId, &table](MemDb::Row from, MemDb::Row to)
    {
        Game::Entity fromEntity = ((Game::Entity*)(table.columns.Get<1>(ownerColumnId.id)))[from];
        Game::Entity toEntity = ((Game::Entity*)(table.columns.Get<1>(ownerColumnId.id)))[to];
        if (!IsValid(world, fromEntity))
        {
            // we need to add these instances new index to the to the freeids list, since it's been deleted.
            // the 'from' instance will be swapped with the 'to' instance, so we just add the 'to' id to the list;
            // and it will automatically be defragged
            table.freeIds.Append(to);
        }
        else if (world->entityMap[fromEntity.index].category == world->entityMap[toEntity.index].category)
        {
            // just swap the instances
            world->entityMap[fromEntity.index].instance = to;
            world->entityMap[toEntity.index].instance = from;
        }
        else
        {
            // if the entities does not belong to the same category, only update the
            // instance of the one that has been moved.
            // This is most likely due to an entity migration
            world->entityMap[fromEntity.index].instance = to;
        }
    });
}

//------------------------------------------------------------------------------
/**
*/
Entity
AllocateEntity(World* world)
{
    Entity entity;
    if (world->pool.Allocate(entity))
    {
        world->entityMap.Append({ MemDb::InvalidTableId, MemDb::InvalidRow });
    }
    world->numEntities++;
    return entity;
}

//------------------------------------------------------------------------------
/**
*/
void
DeallocateEntity(World* world, Entity entity)
{
    n_assert(!IsActive(world, entity));
    world->pool.Deallocate(entity);
    world->numEntities--;
}

//------------------------------------------------------------------------------
/**
*/
void
SetProperty(World* world, Game::Entity entity, Game::PropertyId pid, void* value, uint64_t size)
{
#if NEBULA_DEBUG
    n_assert2(size == MemDb::TypeRegistry::TypeSize(pid), "SetProperty: Provided value's type is not the correct size for the given PropertyId.");
#endif
    EntityMapping mapping = GetEntityMapping(world, entity);
    byte* const ptr = (byte*)GetInstanceBuffer(world, mapping.category, pid);
    byte* valuePtr = ptr + (mapping.instance * size);
    Memory::Copy(value, valuePtr, size);
}

//------------------------------------------------------------------------------
/**
*/
PropertyDecayBuffer const
GetDecayBuffer(Game::PropertyId pid)
{
    if (pid < propertyDecayTable.Size())
        return propertyDecayTable[pid.id];
    else
        return PropertyDecayBuffer();
}

//------------------------------------------------------------------------------
/**
*/
void
ClearDecayBuffers()
{
    for (auto& pdb : propertyDecayTable)
    {
        // TODO: shrink buffers if they're unreasonably big.
        pdb.size = 0;
    }
}

} // namespace Game
