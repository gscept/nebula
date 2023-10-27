#pragma once
//------------------------------------------------------------------------------
/**
    @file world.h

    @copyright
    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "entity.h"
#include "entitypool.h"
#include "component.h"
#include "memdb/tablesignature.h"
#include "util/queue.h"
#include "category.h"
#include "processorid.h"
#include "processor.h"

namespace MemDb { class Database; }

namespace Game
{

using OpBuffer = uint32_t;

namespace Op
{

//------------------------------------------------------------------------------
/**
    TODO: We should probably rewrite the entire op system, since it's a mess at the moment.
          Also, try to clean up the world system, so that it makes more sense, and is less dependant on other systems and callsites to set things up...
            Trying to setup a world manually right now is almost impossible.
*/
struct RegisterComponent
{
    Entity entity;
    ComponentId component;
    void const* value = nullptr;
};

//------------------------------------------------------------------------------
/**
*/
struct DeregisterComponent
{
    Entity entity;
    ComponentId component;
};

} // namespace Op

//------------------------------------------------------------------------------
/**
*/
struct EntityCreateInfo
{
    /// template to instantiate.
    TemplateId templateId = TemplateId::Invalid();
    /// set if the entity should be instantiated immediately or deferred until end of frame.
    bool immediate = false;
};

//------------------------------------------------------------------------------
/**
*/
class World
{
public:
    ~World();

    /// register a component event listener
    void RegisterComponentEvent(ComponentEvent const& event);

    /// Create a new empty entity
    Game::Entity CreateEntity();
    /// Create a new entity from create info
    Game::Entity CreateEntity(EntityCreateInfo const& info);
    /// Delete entity
    void DeleteEntity(Game::Entity entity);
    /// Check if an entity ID is still valid.
    bool IsValid(Entity e);
    /// Check if an entity has an instance. It might be valid, but not have received an instance just after it has been created.
    bool HasInstance(Entity e);
    /// Returns the entity mapping of an entity
    EntityMapping GetEntityMapping(Game::Entity entity);
    /// Check if entity has a specific component. (SLOW!)
    bool HasComponent(Game::Entity const entity, ComponentId const component);
    /// Get instance of entity
    MemDb::RowId GetInstance(Entity entity);
    /// Add a component to an entity.
    template <typename TYPE>
    void AddComponent(Entity, TYPE* prop);
    /// Remove a component from an entity
    template <typename TYPE>
    void RemoveComponent(Entity);
    /// Set the value of an entitys component
    template <typename TYPE>
    void SetComponent(Entity entity, TYPE value);
    /// Get an entitys component
    template <typename TYPE>
    TYPE GetComponent(Entity entity);

    /// Get a decay buffer for the given component
    ComponentDecayBuffer const GetDecayBuffer(Game::ComponentId component);
    
    // TODO: rewrite the op buffer system...
    OpBuffer GetScratchOpBuffer();
    OpBuffer CreateOpBuffer();
    void Dispatch(OpBuffer buffer);
    void DestroyOpBuffer(OpBuffer& buffer);
    void AddOp(OpBuffer buffer, Op::RegisterComponent op);
    void AddOp(OpBuffer buffer, Op::DeregisterComponent const& op);
    void Execute(Op::RegisterComponent const& op);
    void Execute(Op::DeregisterComponent const& op);
    void ReleaseAllOps();

    /// Create a table in the entity database that has a specific set of components
    MemDb::TableId CreateEntityTable(CategoryCreateInfo const& info);
    /// Register a number of processors to the world
    void RegisterProcessors(std::initializer_list<ProcessorHandle> handles);
    /// Set the value of a component by providing a pointer and type size
    void SetComponentValue(World* world, Game::Entity entity, Game::ComponentId component, void* value, uint64_t size);
    
    /// Query the entity database using specified filter set. This does NOT wait for resources to be available.
    Dataset Query(Filter filter);
    /// Query a subset of tables using a specified filter set. Modifies the tables array so that it only contains valid tables.
    /// This does NOT wait for resources to be available.
    Dataset Query(Filter filter, Util::Array<MemDb::TableId>& tids);

    /// Get the entity database. Be careful when directly modifying the database, as some information is only kept track of via the World.
    Ptr<MemDb::Database> GetDatabase();

    /// copies and overrides dst with src. This is extremely destructive - make sure you understand the implications!
    static void Override(World* src, World* dst);
private:
    friend class GameServer;
    friend class BlueprintManager;

    // Only the game server can create worlds
    World(uint32_t hash);

    // These functions are called from game server
    void BeginFrame();
    void SimFrame();
    void EndFrame();
    void OnLoad();
    void OnSave();
    void RenderDebug();
    void ManageEntities();
    void Reset();
    void PrefilterProcessors();
    /// Check if the database is fully prefiltered.
    bool Prefiltered() const;
    /// Clears all decay buffers. This is called by the game server automatically.
    void ClearDecayBuffers();

    /// Get total number of instances in an entity table
    SizeT GetNumInstances(MemDb::TableId tid);
    /// Get a pointer to the first instance of a component in a partition of an entity table.
    void* GetInstanceBuffer(MemDb::TableId const tid, uint16_t partitionId, ComponentId const component);

    Entity AllocateEntity();
    void DeallocateEntity(Entity entity);
    void DecayComponent(Game::ComponentId component, MemDb::TableId tableId, MemDb::ColumnIndex column, MemDb::RowId instance);

    MemDb::RowId AllocateInstance(Entity entity, MemDb::TableId table);
    MemDb::RowId AllocateInstance(Entity entity, BlueprintId blueprint);
    MemDb::RowId AllocateInstance(Entity entity, TemplateId templateId);
    void DeallocateInstance(MemDb::TableId table, MemDb::RowId instance);
    void DeallocateInstance(Entity entity);
    MemDb::RowId Migrate(Entity entity, MemDb::TableId newCategory);
    void Migrate(
        Util::Array<Entity> const& entities,
        MemDb::TableId fromCategory,
        MemDb::TableId newCategory,
        Util::FixedArray<MemDb::RowId>& newInstances
    );

    /// Defragment an entity table
    void Defragment(MemDb::TableId cat);


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
    Util::Array<CallbackInfo> onActivateCallbacks;
    CallbackInfo activateAllInstancesCallback;

    /// set to true if the caches for the callbacks are invalid
    bool cacheValid = false;
};

//------------------------------------------------------------------------------
/**
*/
template <typename TYPE>
inline void
World::SetComponent(Entity entity, TYPE value)
{
#if NEBULA_DEBUG
    n_assert2(
        sizeof(TYPE) == MemDb::AttributeRegistry::TypeSize(Game::GetComponentId<TYPE>()),
        "SetComponent: Provided value's type is not the correct size for the given ComponentId."
    );
#endif
    EntityMapping mapping = this->GetEntityMapping(entity);
    TYPE* ptr = (TYPE*)this->GetInstanceBuffer(mapping.table, mapping.instance.partition, GetComponentId<TYPE>());
    *(ptr + mapping.instance.index) = value;
}

//------------------------------------------------------------------------------
/**
*/
template <typename TYPE>
inline TYPE
World::GetComponent(Entity entity)
{
#if NEBULA_DEBUG
    n_assert2(
        sizeof(TYPE) == MemDb::AttributeRegistry::TypeSize(Game::GetComponentId<TYPE>()),
        "GetComponent: Provided value's type is not the correct size for the given ComponentId."
    );
#endif
    EntityMapping mapping = this->GetEntityMapping(entity);
    TYPE* ptr = (TYPE*)this->GetInstanceBuffer(mapping.table, mapping.instance.partition, GetComponentId<TYPE>());
    return *(ptr + mapping.instance.index);
}

//------------------------------------------------------------------------------
/**
*/
template <typename TYPE>
inline void
World::AddComponent(Entity entity, TYPE* value)
{
    //n_assert(!state.asyncProcessing);
    Op::RegisterComponent op;
    op.entity = entity;
    op.component = Game::GetComponentId<TYPE>();
    op.value = (void*)value;
    this->AddOp(this->GetScratchOpBuffer(), op);
}

//------------------------------------------------------------------------------
/**
*/
template <typename TYPE>
inline void
World::RemoveComponent(Entity entity)
{
    //n_assert(!state.asyncProcessing);
    Op::DeregisterComponent op;
    op.entity = entity;
    op.component = GetComponentId<TYPE>();
    this->AddOp(this->GetScratchOpBuffer(), op);
}

} // namespace Game
