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

namespace Game
{

//------------------------------------------------------------------------------
using RegCidQueue = Util::Queue<Op::RegisterComponent>;
using DeregCidQueue = Util::Queue<Op::DeregisterComponent>;

static Ids::IdAllocator<World*, RegCidQueue, DeregCidQueue> opBufferAllocator;
static Memory::ArenaAllocator<1024> opAllocator;

static Util::FixedArray<ComponentDecayBuffer> componentDecayTable;

//------------------------------------------------------------------------------
/**
*/
class World
{
public:
    World();
    ~World();

    struct AllocateInstanceCommand
    {
        Game::Entity entity;
        TemplateId tid;
    };
    struct DeallocInstanceCommand
    {
        Game::Entity entity;
    };

    /// used to allocate entity ids for this world
    EntityPool pool;
    /// Number of entities alive
    SizeT numEntities;
    /// maps entity index to table+instanceid pair
    Util::Array<Game::EntityMapping> entityMap;
    /// contains all entity instances
    Ptr<MemDb::Database> db;
    /// world hash
    uint32_t hash;
    /// maps from blueprint to a table that has the same signature
    Util::HashTable<BlueprintId, MemDb::TableId> blueprintCatMap;
    ///
    Util::Queue<AllocateInstanceCommand> allocQueue;
    ///
    Util::Queue<DeallocInstanceCommand> deallocQueue;
    /// synchronous op buffer that can be used by any sync processor
    OpBuffer scratchOpBuffer;

    /// add the table to any callback-caches that accepts it
    void CacheTable(MemDb::TableId tid, MemDb::TableSignature signature);

    struct CallbackInfo
    {
        ProcessorHandle handle;
        Filter filter;
        ProcessorFrameCallback func;
        /// cached tables that we've filtered out.
        Util::Array<MemDb::TableId> cache;
    };

    Util::Array<CallbackInfo> onBeginFrameCallbacks;
    Util::Array<CallbackInfo> onFrameCallbacks;
    Util::Array<CallbackInfo> onEndFrameCallbacks;
    Util::Array<CallbackInfo> onLoadCallbacks;
    Util::Array<CallbackInfo> onSaveCallbacks;

    /// set to true if the caches for the callbacks are invalid
    bool cacheValid = false;
};

//------------------------------------------------------------------------------
/**
*/
World::World() :
    numEntities(0)
{
    this->db = MemDb::Database::Create();
    this->scratchOpBuffer = Game::CreateOpBuffer(this);
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
World*
AllocateWorld(WorldCreateInfo const& info)
{
    World* world = new World;
    world->hash = info.hash;
    return world;
}

//------------------------------------------------------------------------------
/**
*/
void
DeallocateWorld(World* world)
{
    delete world;
}

//------------------------------------------------------------------------------
/**
*/
void
WorldBeginFrame(World* world)
{
    int const num = world->onBeginFrameCallbacks.Size();
    for (int i = 0; i < num; i++)
    {
        Dataset data = Game::Query(world, world->onBeginFrameCallbacks[i].cache, world->onBeginFrameCallbacks[i].filter);
        world->onBeginFrameCallbacks[i].func(world, data);
    }
    Game::Dispatch(world->scratchOpBuffer);
}

//------------------------------------------------------------------------------
/**
*/
void
WorldSimFrame(World* world)
{
    int const num = world->onFrameCallbacks.Size();
    for (int i = 0; i < num; i++)
    {
        Dataset data = Game::Query(world, world->onFrameCallbacks[i].cache, world->onFrameCallbacks[i].filter);
        world->onFrameCallbacks[i].func(world, data);
    }
    Game::Dispatch(world->scratchOpBuffer);
}

//------------------------------------------------------------------------------
/**
*/
void
WorldEndFrame(World* world)
{
    int const num = world->onEndFrameCallbacks.Size();
    for (int i = 0; i < num; i++)
    {
        Dataset data = Game::Query(world, world->onEndFrameCallbacks[i].cache, world->onEndFrameCallbacks[i].filter);
        world->onEndFrameCallbacks[i].func(world, data);
    }
    Game::Dispatch(world->scratchOpBuffer);
}

//------------------------------------------------------------------------------
/**
*/
void
WorldOnLoad(World* world)
{
    int num = world->onLoadCallbacks.Size();
    for (int i = 0; i < num; i++)
    {
        Dataset data = Game::Query(world, world->onLoadCallbacks[i].cache, world->onLoadCallbacks[i].filter);
        world->onLoadCallbacks[i].func(world, data);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
WorldOnSave(World* world)
{
    int num = world->onSaveCallbacks.Size();
    for (int i = 0; i < num; i++)
    {
        Dataset data = Game::Query(world, world->onSaveCallbacks[i].cache, world->onSaveCallbacks[i].filter);
        world->onSaveCallbacks[i].func(world, data);    
    }
}

//------------------------------------------------------------------------------
/**
*/
void
WorldPrefilterProcessors(World* world)
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

    world->cacheValid = true;
}

//------------------------------------------------------------------------------
/**
*/
bool
WorldPrefiltered(World* world)
{
    return world->cacheValid;
}

//------------------------------------------------------------------------------
/**
*/
void
WorldManageEntities(World* world)
{
    // NOTE: The order of the following loops are important!

    // Clean up entities
    while (!world->deallocQueue.IsEmpty())
    {
        auto const cmd = world->deallocQueue.Dequeue();
        if (Game::IsValid(world, cmd.entity))
        {
            MemDb::TableId const table = world->entityMap[cmd.entity.index].table;
            MemDb::Row const row = world->entityMap[cmd.entity.index].instance;
            DeallocateInstance(world, table, row);
            world->entityMap[cmd.entity.index].table = MemDb::InvalidTableId;
            world->entityMap[cmd.entity.index].instance = MemDb::InvalidRow;
            DeallocateEntity(world, cmd.entity);
        }
    }

    // Allocate instances for new entities, reuse invalid instances if possible
    while (!world->allocQueue.IsEmpty())
    {
        auto const cmd = world->allocQueue.Dequeue();
        n_assert(IsValid(world, cmd.entity));
        AllocateInstance(world, cmd.entity, cmd.tid);
    }

    // Delete all remaining invalid instances
    Ptr<MemDb::Database> const& db = world->db;

    if (db.isvalid())
    {
        db->ForEachTable([world](MemDb::TableId tid)
            {
                Defragment(world, tid);
            });
    }
}

//------------------------------------------------------------------------------
/**
*/
void
WorldReset(World* world)
{
    world->db->Reset();
}

//------------------------------------------------------------------------------
/**
*/
OpBuffer
WorldGetScratchOpBuffer(World* world)
{
    return world->scratchOpBuffer;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<MemDb::Database>
GetWorldDatabase(World* world)
{
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
DecayComponent(Game::World* world, Game::ComponentId component, MemDb::TableId tableId, MemDb::ColumnIndex column, MemDb::Row instance)
{
    if (MemDb::TypeRegistry::Flags(component) & ComponentFlags::COMPONENTFLAG_MANAGED)
    {
        if (component.id >= componentDecayTable.Size())
            componentDecayTable.Resize(component.id + 16); // increment with a couple of extra elements, instead of doubling size, just to avoid extreme overallocation
        ComponentDecayBuffer& pdb = componentDecayTable[component.id];

        uint64_t const typeSize = (uint64_t)MemDb::TypeRegistry::TypeSize(component);

        if (pdb.capacity == 0)
        {
            pdb.size = 0;
            pdb.capacity = 64;
            pdb.buffer = Memory::Alloc(Memory::HeapType::DefaultHeap, pdb.capacity * typeSize);
        }

        if (pdb.capacity == pdb.size)
        {
            void* oldBuffer = pdb.buffer;
            pdb.capacity *= 2;
            pdb.buffer = Memory::Alloc(Memory::HeapType::DefaultHeap, pdb.capacity * typeSize);
            Memory::Copy(oldBuffer, pdb.buffer, typeSize * (uint64_t)pdb.size);
            Memory::Free(Memory::HeapType::DefaultHeap, oldBuffer);
        }

        void* dst = ((byte*)pdb.buffer) + (typeSize * (uint64_t)pdb.size);
        pdb.size++;
        Memory::Copy(world->db->GetValuePointer(tableId, column, instance), dst, typeSize);
    }
}


//------------------------------------------------------------------------------
/**
*/
ComponentDecayBuffer const
GetDecayBuffer(Game::ComponentId component)
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
ClearDecayBuffers()
{
    for (auto& pdb : componentDecayTable)
    {
        // TODO: shrink buffers if they're unreasonably big.
        pdb.size = 0;
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
    @todo   We can bundle all add and remove components for each entity into one
            migration.
            We can also batch them based on their new table, so we won't
            need to do as many column id lookups.
    @note   This is not thread safe. We cannot make it sync since it might read/write from arbitrary tables.
*/
void
Dispatch(OpBuffer buffer)
{
    World* world = opBufferAllocator.Get<0>(buffer);
    RegCidQueue& registerComponentQueue = opBufferAllocator.Get<1>(buffer);
    DeregCidQueue& deregisterComponentQueue = opBufferAllocator.Get<2>(buffer);

    while (!registerComponentQueue.IsEmpty())
    {
        auto op = registerComponentQueue.Dequeue();
        Game::Execute(world, op);
    }

    while (!deregisterComponentQueue.IsEmpty())
    {
        auto op = deregisterComponentQueue.Dequeue();
        Game::Execute(world, op);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyOpBuffer(OpBuffer& buffer)
{
    opBufferAllocator.Dealloc(buffer);
    buffer = InvalidIndex;
}

//------------------------------------------------------------------------------
/**
    @todo   optimize
*/
void
AddOp(OpBuffer buffer, Op::RegisterComponent op)
{
    if (op.value != nullptr)
    {
        SizeT const typeSize = MemDb::TypeRegistry::TypeSize(op.component);
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
AddOp(OpBuffer buffer, Op::DeregisterComponent const& op)
{
    opBufferAllocator.Get<2>(buffer).Enqueue(op);
}

//------------------------------------------------------------------------------
/**
    @todo   Optimize me
*/
void
Execute(World* world, Op::RegisterComponent const& op)
{
    EntityMapping const mapping = GetEntityMapping(world, op.entity);

    MemDb::TableSignature signature = world->db->GetTableSignature(mapping.table);
    if (signature.IsSet(op.component))
        return;

    signature.FlipBit(op.component);

    MemDb::TableId newCategoryId = world->db->FindTable(signature);
    if (newCategoryId == MemDb::InvalidTableId)
    {
        CategoryCreateInfo info;
        auto const& cols = world->db->GetTable(mapping.table).components;
        info.components.SetSize(cols.Size() + 1);
        IndexT i;
        for (i = 0; i < cols.Size(); ++i)
        {
            info.components[i] = cols[i];
        }
        info.components[i] = op.component;

        newCategoryId = CreateEntityTable(world, info);
    }

    MemDb::Row newInstance = Migrate(world, op.entity, newCategoryId);

    if (op.value == nullptr)
        return; // default value should already be set

    auto cid = world->db->GetColumnId(newCategoryId, op.component);
    void* ptr = world->db->GetValuePointer(newCategoryId, cid, newInstance);
    Memory::Copy(op.value, ptr, MemDb::TypeRegistry::TypeSize(op.component));
}

//------------------------------------------------------------------------------
/**
    @bug   If you deregister a managed component, the component will just disappear
           without letting the manager clean up any resources, leading to memleaks.

            Possible fix: We can change the "decay system" to have a decay buffer
            for each component that is managed. That way, when an entity is deleted,
            or a component is deregistered, we copy the managed components into their
            respective decay buffer. We can create a separate frame event for decay
            handling as well. This means each table won't have their own decay tables,
            but each component will have a decay buffer instead that is not a fully fledged
            table. The buffers won't care about which entities owned the instances.
            The decay buffers should be cleaned at the specific decay cleaup event each
            frame.
*/
void
Execute(World* world, Op::DeregisterComponent const& op)
{
#if NEBULA_DEBUG
    n_assert(Game::HasComponent(world, op.entity, op.component));
#endif
    EntityMapping const mapping = GetEntityMapping(world, op.entity);
    MemDb::TableSignature signature = world->db->GetTableSignature(mapping.table);
    if (!signature.IsSet(op.component))
        return;

    signature.FlipBit(op.component);

    MemDb::TableId newCategoryId = world->db->FindTable(signature);
    if (newCategoryId == MemDb::InvalidTableId)
    {
        CategoryCreateInfo info;
        auto const& cols = world->db->GetTable(mapping.table).components;
        SizeT const num = cols.Size();
        info.components.SetSize(num - 1);
        int col = 0;
        for (int i = 0; i < num; ++i)
        {
            if (cols[i] == op.component)
                continue;

            info.components[col++] = cols[i];
        }

        newCategoryId = CreateEntityTable(world, info);
    }

    DecayComponent(world, op.component, mapping.table, world->db->GetColumnId(mapping.table, op.component), mapping.instance);

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
   TODO: This is not thread safe!
*/
bool
HasComponent(World* world, Game::Entity const entity, ComponentId const component)
{
    EntityMapping mapping = GetEntityMapping(world, entity);
    return world->db->HasComponent(mapping.table, component);
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
GetInstanceBuffer(World* world, MemDb::TableId const tid, ComponentId const component)
{
    Ptr<MemDb::Database> db = world->db;
    auto cid = db->GetColumnId(tid, component);
#if NEBULA_DEBUG
    n_assert_fmt(cid != MemDb::ColumnIndex::Invalid(), "GetInstanceBuffer: Entity table does not have component with id '%i'!\n", component.id);
#endif
    return db->GetBuffer(tid, cid);
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
    MemDb::TableSignature signature(info.components);

    MemDb::TableId categoryId = world->db->FindTable(signature);
    if (categoryId != MemDb::TableId::Invalid())
    {
        return categoryId;
    }

    constexpr ushort NUM_PROPS = 256;
    ComponentId components[NUM_PROPS];

    MemDb::TableCreateInfo tableInfo;
    tableInfo.name = info.name;
    if (info.components[0] != GameServer::Singleton->state.ownerId)
    {
        // push owner id into the component array
        const SizeT tableSize = 1 + info.components.Size();
        n_assert(tableSize < NUM_PROPS);
        tableInfo.numComponents = tableSize;
        tableInfo.components = components;

        // always add owner as first column
        components[0] = GameServer::Singleton->state.ownerId;
        for (int i = 1; i < tableSize; i++)
        {
            components[i] = info.components[i - 1];
        }
    }
    else
    {
        const SizeT tableSize = info.components.Size();
        tableInfo.numComponents = tableSize;
        tableInfo.components = info.components.begin();
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
AllocateInstance(World* world, Entity entity, MemDb::TableId table)
{
    n_assert(world->pool.IsValid(entity));
    n_assert(world->entityMap[entity.index].instance == MemDb::InvalidRow);

    if (entity.index < world->entityMap.Size() && world->entityMap[entity.index].instance != MemDb::InvalidRow)
    {
        n_warning("Entity already registered!\n");
        return InvalidIndex;
    }

    MemDb::Row instance = world->db->AllocateRow(table);

    world->entityMap[entity.index] = { table, instance };

    // Just make sure the first column in always owner!
    n_assert(world->db->GetColumnId(table, GameServer::Singleton->state.ownerId) == 0);

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)world->db->GetBuffer(table, 0);
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
    n_assert(world->db->GetColumnId(mapping.table, GameServer::Singleton->state.ownerId) == 0);

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)world->db->GetBuffer(mapping.table, 0);
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
    Game::Entity* owners = (Game::Entity*)world->db->GetBuffer(mapping.table, 0);
    owners[mapping.instance] = entity;

    return mapping.instance;
}

//------------------------------------------------------------------------------
/**
*/
void
DeallocateInstance(World* world, MemDb::TableId table, MemDb::Row instance)
{
    n_assert(instance != MemDb::InvalidRow);

    // migrate managed properies to decay buffers so that we can allow the managers
    // to clean up any externally allocated resources.
    Util::Array<ComponentId> const& pids = world->db->GetTable(table).columns.GetArray<0>();
    const MemDb::ColumnIndex numColumns = pids.Size();
    for (MemDb::ColumnIndex column = 0; column < numColumns.id; column.id++)
    {
        Game::ComponentId component = pids[column.id];
        DecayComponent(world, component, table, column, instance);
    }

    world->db->DeallocateRow(table, instance);
}

//------------------------------------------------------------------------------
/**
*/
void
DeallocateInstance(World* world, Entity entity)
{
    MemDb::TableId& table = world->entityMap[entity.index].table;
    MemDb::Row& instance = world->entityMap[entity.index].instance;

    DeallocateInstance(world, table, instance);

    table = MemDb::InvalidTableId;
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
    MemDb::Row newInstance = world->db->MigrateInstance(mapping.table, mapping.instance, newCategory, false);
    Defragment(world, mapping.table);
    world->entityMap[entity.index] = { newCategory, newInstance };
    return newInstance;
}

//------------------------------------------------------------------------------
/**
    @param newInstances     Will be filled with the new instance ids in the destination table.
    @note   This assumes ALL entities in the entity array is of same table!
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
        n_assert(mapping.table == fromCategory);
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

    world->cacheValid = false;
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
            else if (world->entityMap[fromEntity.index].table == world->entityMap[toEntity.index].table)
            {
                // just swap the instances
                world->entityMap[fromEntity.index].instance = to;
                world->entityMap[toEntity.index].instance = from;
            }
            else
            {
                // if the entities does not belong to the same table, only update the
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
SetComponent(World* world, Game::Entity entity, Game::ComponentId component, void* value, uint64_t size)
{
#if NEBULA_DEBUG
    n_assert2(size == MemDb::TypeRegistry::TypeSize(component), "SetComponent: Provided value's type is not the correct size for the given ComponentId.");
#endif
    EntityMapping mapping = GetEntityMapping(world, entity);
    byte* const ptr = (byte*)GetInstanceBuffer(world, mapping.table, component);
    byte* valuePtr = ptr + (mapping.instance * size);
    Memory::Copy(value, valuePtr, size);
}

//------------------------------------------------------------------------------
/**
*/
EntityMapping
BlueprintManager::Instantiate(World* const world, BlueprintId blueprint)
{
    GameServer::State& gsState = GameServer::Instance()->state;
    Ptr<MemDb::Database> const& tdb = gsState.templateDatabase;
    IndexT const categoryIndex = world->blueprintCatMap.FindIndex(blueprint);

    if (categoryIndex != InvalidIndex)
    {
        MemDb::TableId const cid = world->blueprintCatMap.ValueAtIndex(blueprint, categoryIndex);
        MemDb::Row const instance = world->db->AllocateRow(cid);
        return { cid, instance };
    }
    else
    {
        // Create the table, and then create the instance
        MemDb::TableId const cid = this->CreateCategory(world, blueprint);
        MemDb::Row const instance = world->db->AllocateRow(cid);
        return { cid, instance };
    }
}

//------------------------------------------------------------------------------
/**
*/
EntityMapping
BlueprintManager::Instantiate(World* const world, TemplateId templateId)
{
    n_assert(Singleton->templateIdPool.IsValid(templateId.id));
    GameServer::State& gsState = GameServer::Instance()->state;
    Ptr<MemDb::Database> const& tdb = gsState.templateDatabase;
    Template& tmpl = Singleton->templates[Ids::Index(templateId.id)];
    IndexT const categoryIndex = world->blueprintCatMap.FindIndex(tmpl.bid);

    if (categoryIndex != InvalidIndex)
    {
        MemDb::TableId const tid = world->blueprintCatMap.ValueAtIndex(tmpl.bid, categoryIndex);
        MemDb::Row const instance = tdb->DuplicateInstance(Singleton->blueprints[tmpl.bid.id].tableId, tmpl.row, world->db, tid);
        return { tid, instance };
    }
    else
    {
        // Create the table, and then create the instance
        MemDb::TableId const tid = this->CreateCategory(world, tmpl.bid);
        MemDb::Row const instance = tdb->DuplicateInstance(Singleton->blueprints[tmpl.bid.id].tableId, tmpl.row, world->db, tid);
        return { tid, instance };
    }
}

//------------------------------------------------------------------------------
/**
    @todo   this can be optimized
*/
MemDb::TableId
BlueprintManager::CreateCategory(World* const world, BlueprintId bid)
{
    CategoryCreateInfo info;
    info.name = blueprints[bid.id].name.Value();

    auto const& components = GameServer::Singleton->state.templateDatabase->GetTable(blueprints[bid.id].tableId).components;
    info.components.Resize(components.Size());
    for (int i = 0; i < components.Size(); i++)
    {
        info.components[i] = components[i];
    }

    MemDb::TableId tid = CreateEntityTable(world, info);
    world->blueprintCatMap.Add(bid, tid);
    return tid;
}

//------------------------------------------------------------------------------
/**
*/
void
WorldRenderDebug(World* world)
{
    ImGui::Text("World Hash: %s", Util::FourCC(world->hash).AsString().AsCharPtr());
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
        for (auto const& callback : world->onBeginFrameCallbacks)
            PrintCallbackInfo(callback);

        ImGui::TextDisabled("-- OnFrame --");
        for (auto const& callback : world->onFrameCallbacks)
            PrintCallbackInfo(callback);

        ImGui::TextDisabled("-- OnEndFrame --");
        for (auto const& callback : world->onEndFrameCallbacks)
            PrintCallbackInfo(callback);

        ImGui::TextDisabled("-- OnSave --");
        for (auto const& callback : world->onSaveCallbacks)
            PrintCallbackInfo(callback);

        ImGui::TextDisabled("-- OnLoad --");
        for (auto const& callback : world->onLoadCallbacks)
            PrintCallbackInfo(callback);

        ImGui::Separator();
    }

    static bool listInactive = false;
    ImGui::Checkbox("List inactive instances", &listInactive);
    ImGui::Text("Entity map:");
    ImGui::BeginChild("ScrollingRegion", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()), false, ImGuiWindowFlags_HorizontalScrollbar);
    {
        for (uint entityIndex = 0; entityIndex < world->entityMap.Size(); entityIndex++)
        {
            Game::EntityMapping entity = world->entityMap[entityIndex];

            if (!listInactive && (entity.table == MemDb::InvalidTableId || entity.instance == MemDb::InvalidRow))
                continue;

            ImGui::BeginGroup();
            ImGui::Text("[%i] ", entityIndex);
            ImGui::SameLine();
            ImGui::TextColored({ 1,0.3f,0,1 }, "tid:%i, row:%i", entity.table, entity.instance);
            if (entity.table != MemDb::TableId::Invalid())
            {
                ImGui::TextDisabled("- %s", Game::GetWorldDatabase(world)->GetTable(entity.table).name.Value());
                ImGui::EndGroup();
                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::TextDisabled("- %s", Game::GetWorldDatabase(world)->GetTable(entity.table).name.Value());
                    MemDb::TableId const table = entity.table;
                    MemDb::Row const row = entity.instance;

                    auto const& components = Game::GetWorldDatabase(world)->GetTable(table).components;
                    for (auto component : components)
                    {
                        SizeT const typeSize = MemDb::TypeRegistry::TypeSize(component);
                        if (typeSize == 0)
                        {
                            // Type is flag type, just print the name, and then continue
                            ImGui::Text("_flag_: %s", MemDb::TypeRegistry::GetDescription(component)->name.Value());
                            ImGui::Separator();
                            continue;
                        }
                        void* data = Game::GetInstanceBuffer(world, table, component);
                        data = (byte*)data + (row * typeSize);
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
WorldOverride(World* src, World* dst)
{
    dst->blueprintCatMap = src->blueprintCatMap;
    dst->entityMap = src->entityMap;
    dst->numEntities = src->numEntities;
    dst->pool = src->pool;
    dst->db = nullptr;
    dst->db = MemDb::Database::Create();
    src->db->Copy(dst->db);

    WorldPrefilterProcessors(dst);
}

} // namespace Game
