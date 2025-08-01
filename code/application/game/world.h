#pragma once
//------------------------------------------------------------------------------
/**
    @file world.h

    @copyright
    (C) 2021-2024 Individual contributors, see AUTHORS file
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
#include "memory/arenaallocator.h"
#include "frameevent.h"
#include "util/fourcc.h"

namespace MemDb
{
class Database;
}
namespace Util
{
class Blob;
}

namespace Game
{

class PackedLevel;

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

/// returns a world by hash
World* GetWorld(WorldHash worldHash);
/// returns a world by id
World* GetWorld(WorldId worldId);

//------------------------------------------------------------------------------
/**
    @class Game::World

    @brief A container of entities, their components, and processors.
    
    @details Worlds can be identified by their hash, or their id. 
    
    A game world keeps track of: 
    
    @li Entities (Game::Entity)
    @li Components
    @li Processors (Game::Processor) 
    @li A game frame pipeline (Game::FramePipeline)

    Components are stored as columns in a database, while an entity maps to
    a row of multiple component columns, in a table. The database splits 
    entities based on their components, so all entities with the same
    components are stored in the same table. "Adding or removing" a
    component from an entity means moving it from one table to another.

    Processors are functions that process entity components data. They loop
    over all entities that fulfill some condition of having certain components
    or not, and runs a user defined processing function over this data.

    The game frame pipeline stores all the processor in order of execution.
*/
class World
{
public:
    // Generally, only the game server should create worlds
    World(WorldHash hash, WorldId id);
    ~World();

    /// Returns the world hash for this world.
    WorldHash GetWorldHash() const;
    /// Returns the world ID for this world. This corresponds to Game::Entity::world.
    WorldId GetWorldId() const;

    /// Create a new empty entity
    Entity CreateEntity(bool immediate = true);
    /// Create a new entity from create info
    Entity CreateEntity(EntityCreateInfo const& info);
    /// Delete entity
    void DeleteEntity(Entity entity);

    /// Check if an entity ID is still valid.
    bool IsValid(Entity entity) const;
    /// Check if an entity has an instance. It might be valid, but not have received an instance just after it has been created.
    bool HasInstance(Entity entity) const;

    /// Returns the entity mapping of an entity
    EntityMapping GetEntityMapping(Entity entity) const;

    /// Create a component. This queues the component in a command buffer to be added later
    template <typename TYPE>
    TYPE* AddComponent(Entity entity);
    /// Add a component to an entity. This queues the component in a command buffer to be added later
    /// This version of AddComponent allows you to set the values of the component before the component is initialized.
    template <typename TYPE>
    void AddComponent(Entity entity, TYPE const& component);
    /// Queues a component to be added to the entity in a command buffer.
    void* AddComponent(Entity entity, ComponentId component);

    /// Check if entity has a specific component.
    template <typename TYPE>
    bool HasComponent(Entity entity) const;
    /// Check if entity has a specific component. (SLOW!)
    bool HasComponent(Entity entity, ComponentId component) const;
    
    /// Remove a component from an entity
    template <typename TYPE>
    void RemoveComponent(Entity entity);
    /// Remove a component from an entity
    void RemoveComponent(Entity entity, ComponentId component);

    /// Set the value of an entitys component
    template <typename TYPE>
    void SetComponent(Entity entity, TYPE const& value);
    /// Get an entitys component
    template <typename TYPE>
    TYPE GetComponent(Entity entity);

    /// Mark an entity as modified in its table.
    void MarkAsModified(Game::Entity entity);

    /// Query the entity database using specified filter set. This does NOT wait for resources to be available.
    Dataset Query(Filter filter);
    /// Query a subset of tables using a specified filter set. Modifies the tables array so that it only contains valid tables.
    /// This does NOT wait for resources to be available.
    Dataset Query(Filter filter, Util::Array<MemDb::TableId>& tids);

    /// Get a decay buffer for the given component
    ComponentDecayBuffer const GetDecayBuffer(ComponentId component);

    /// preload a level that can be instantiated
    PackedLevel* PreloadLevel(Util::String const& path);
    /// unload a preloaded level
    void UnloadLevel(PackedLevel* level);
    /// Export the world as a level
    void ExportLevel(Util::String const& path);

    /// Get the frame pipeline
    FramePipeline& GetFramePipeline();

    /// Get the entity database. Be careful when directly modifying the database, as some information is only kept track of via the World.
    Ptr<MemDb::Database> GetDatabase();

    // -- Internal methods -- Use with caution! --

    /// Get instance of entity
    MemDb::RowId GetInstance(Entity entity) const;
    /// Set the value of a component by providing a pointer and type size
    void SetComponentValue(Entity entity, ComponentId component, void* value, uint64_t size);
    /// Set the value of a component by providing a pointer and type size, then reinitialize the component
    void ReinitializeComponent(Entity entity, ComponentId component, void* value, uint64_t size);
    /// Create a table in the entity database that has a specific set of components
    MemDb::TableId CreateEntityTable(EntityTableCreateInfo const& info);
    /// copies and overrides dst with src. This is extremely destructive - make sure you understand the implications!
    static void Override(World* src, World* dst);
    /// Allocate an entity id. Use this with caution!
    Entity AllocateEntityId();
    /// Deallocate an entity id. Use this with caution!
    void DeallocateEntityId(Entity entity);
    /// Allocate an entity instance in a table. Use this with caution!
    MemDb::RowId AllocateInstance(Entity entity, MemDb::TableId table, Util::Blob const* const data = nullptr);
    /// Allocate an entity instance from a blueprint. Use this with caution!
    MemDb::RowId AllocateInstance(Entity entity, BlueprintId blueprint);
    /// Allocate an entity instance from a template. Use this with caution!
    MemDb::RowId AllocateInstance(Entity entity, TemplateId templateId, bool performInitialize);
    void FinalizeAllocate(Entity entity);
    /// Deallocate an entity instance. Use this with caution!
    void DeallocateInstance(MemDb::TableId table, MemDb::RowId instance);
    /// Deallocate an entity instance. Use this with caution!
    void DeallocateInstance(Entity entity);
    /// Defragment an entity table
    void Defragment(MemDb::TableId tableId);
    /// Get a pointer to the first instance of a component in a partition of an entity table. Use with caution!
    void* GetInstanceBuffer(MemDb::TableId const tableId, uint16_t partitionId, ComponentId const component);
    /// Get a pointer to the first instance of a column in a partition of an entity table. Use with caution!
    void* GetColumnData(MemDb::TableId const tableId, uint16_t partitionId, MemDb::ColumnIndex const column);
    /// dispatches all staged components to be added to entities
    void ExecuteAddComponentCommands();
    /// Disable if initialization of components is not required (ex. when running as editor db)
    bool componentInitializationEnabled = true;

    void RenderDebug();

private:
    friend class GameServer;
    friend class BlueprintManager;
    friend class PackedLevel;

    struct AllocateInstanceCommand
    {
        Game::Entity entity = Game::Entity::Invalid();
        TemplateId tid = TemplateId::Invalid();
    };

    struct DeallocInstanceCommand
    {
        Game::Entity entity = Game::Entity::Invalid();
    };

    struct AddStagedComponentCommand
    {
        Game::Entity entity = Game::Entity::Invalid();
        ComponentId componentId = ComponentId::Invalid();
        SizeT dataSize = 0;
        void* data = nullptr;
    };

    struct RemoveComponentCommand
    {
        Entity entity = Game::Entity::Invalid();
        ComponentId componentId = ComponentId::Invalid();
    };

    // These functions are called from game server
    void Start();
    void BeginFrame();
    void SimFrame();
    void EndFrame();
    void OnLoad();
    void OnSave();
    void ManageEntities();
    void Reset();
    void PrefilterProcessors();
    /// Clears all decay buffers. This is called by the game server automatically.
    void ClearDecayBuffers();

    void ExecuteRemoveComponentCommands();

    /// Get total number of instances in an entity table
    SizeT GetNumInstances(MemDb::TableId tid);

    /// Migrate an entity from it's current table to a different table
    MemDb::RowId Migrate(Entity entity, MemDb::TableId newTable);
    /// Migrate an array of entities within the same table to a different table
    void Migrate(
        Util::Array<Entity> const& entities,
        MemDb::TableId fromTable,
        MemDb::TableId newTable,
        Util::FixedArray<MemDb::RowId>& newInstances
    );

    /// Copies the component to the decay table
    void DecayComponent(ComponentId component, MemDb::TableId tableId, MemDb::ColumnIndex column, MemDb::RowId instance);

    ///  Move a instance/row within a partition
    void MoveInstance(MemDb::Table::Partition* partition, MemDb::RowId from, MemDb::RowId to);

    /// Run OnInit on all components. Use with caution, since they can only be initialized once and the function doesn't check for this.
    void InitializeAllComponents(Entity entity, MemDb::TableId tableId, MemDb::RowId row);

    /// Adds all components in cmds to entity 
    void AddStagedComponentsToEntity(Entity entity, AddStagedComponentCommand* cmds, SizeT numCmds);
    /// Removes all components in cmds from entity
    void RemoveComponentsFromEntity(Entity entity, RemoveComponentCommand* cmds, SizeT numCmds);

    /// used to allocate entity ids for this world
    EntityPool pool;
    /// Number of entities alive
    SizeT numEntities;
    /// maps entity index to table+row pair
    Util::Array<EntityMapping> entityMap;
    /// contains all entity instances
    Ptr<MemDb::Database> db;
    /// world hash
    WorldHash hash;
    /// world id
    WorldId worldId;
    /// maps from blueprint to a table that has the same signature
    Util::HashTable<BlueprintId, MemDb::TableId> blueprintToTableMap;
    /// Stores all deferred allocation commands
    Util::Queue<AllocateInstanceCommand> allocQueue;
    /// Stores all deferred deallocation commands
    Util::Queue<DeallocInstanceCommand> deallocQueue;
    /// Stores all deferred add staged component commands
    Util::Array<AddStagedComponentCommand> addStagedQueue;
    /// Stores all deferred remove component commands
    Util::Array<RemoveComponentCommand> removeComponentQueue;
    /// Allocator for staged components
    Memory::ArenaAllocator<4096_KB> componentStageAllocator;
    /// Set to true if the caches for the frame pipeline are valid
    bool cacheValid = false;
    /// The frame pipeline for this world
    FramePipeline pipeline;
    /// The default table that empty entities are instantiated into
    MemDb::TableId defaultTableId;
    /// Contains all the component decay buffers. Lookup directly via ComponentId
    Util::FixedArray<ComponentDecayBuffer> componentDecayTable;
};

//------------------------------------------------------------------------------
/**
*/
inline WorldHash
World::GetWorldHash() const
{
    return this->hash;
}

//------------------------------------------------------------------------------
/**
*/
inline WorldId
World::GetWorldId() const
{
    return this->worldId;
}

//------------------------------------------------------------------------------
/**
*/
template <typename TYPE>
inline void
World::SetComponent(Entity entity, TYPE const& value)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Settings components in arbitrary entities while executing an async processor is currently not supported!"
    );
#endif

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
        !this->pipeline.IsRunningAsync(),
        "Getting components from entities while executing an async processor is currently not supported!"
    );
#endif

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
World::RemoveComponent(Entity entity)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(),
        "Removing components from entities while executing an async processor is currently not supported!"
    );
#endif
    RemoveComponentCommand cmd = {
        .entity = entity,
        .componentId = Game::GetComponentId<TYPE>(),
    };
    this->removeComponentQueue.Append(cmd);
}

//------------------------------------------------------------------------------
/**
*/
template <typename TYPE>
inline bool
World::HasComponent(Game::Entity entity) const
{
    return this->HasComponent(entity, Game::GetComponentId<TYPE>());
}

//------------------------------------------------------------------------------
/**
*/
template <typename TYPE>
inline TYPE*
World::AddComponent(Entity entity)
{
    /*
        We could possibly support this by either having thread local allocators and
        staged queues and gathering after the processors have finished (must clear
        allocators by issuing a separate job for this at the end of the frame), or by
        just introducing a simple mutex.
    */
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(), "Adding component to entities while in an async processor is currently not supported!"
    );
#endif
    Game::ComponentId id = Game::GetComponentId<TYPE>();
#if NEBULA_DEBUG
    n_assert(MemDb::AttributeRegistry::TypeSize(id) == sizeof(TYPE));
    //n_assert(!this->HasComponent<TYPE>(entity));
#endif
    TYPE* data = this->componentStageAllocator.Alloc<TYPE>();
    const void* defaultValue = MemDb::AttributeRegistry::DefaultValue(id);
    Memory::Copy(defaultValue, data, sizeof(TYPE));

    AddStagedComponentCommand cmd = {
        .entity = entity,
        .componentId = id,
        .dataSize = sizeof(TYPE),
        .data = data,
    };
    this->addStagedQueue.Append(cmd);

    MemDb::Attribute* attr = MemDb::AttributeRegistry::GetAttribute(id);
    ComponentInterface* cInterface;
    cInterface = static_cast<ComponentInterface*>(attr);

    if (cInterface->Init != nullptr)
    {
        // run initialization function if it exists.
        cInterface->Init(this, entity, data);
    }

    return data;
}

//------------------------------------------------------------------------------
/**
*/
template <typename TYPE>
inline void
World::AddComponent(Entity entity, TYPE const& component)
{
#if NEBULA_DEBUG
    n_assert2(
        !this->pipeline.IsRunningAsync(), "Adding component to entities while in an async processor is currently not supported!"
    );
#endif
    Game::ComponentId id = Game::GetComponentId<TYPE>();
#if NEBULA_DEBUG
    n_assert(MemDb::AttributeRegistry::TypeSize(id) == sizeof(TYPE));
    //n_assert(!this->HasComponent<TYPE>(entity));
#endif
    TYPE* data = this->componentStageAllocator.Alloc<TYPE>();
    Memory::Copy(&component, data, sizeof(TYPE));

    AddStagedComponentCommand cmd = {
        .entity = entity,
        .componentId = id,
        .dataSize = sizeof(TYPE),
        .data = data,
    };
    this->addStagedQueue.Append(cmd);

    MemDb::Attribute* attr = MemDb::AttributeRegistry::GetAttribute(id);
    ComponentInterface* cInterface;
    cInterface = static_cast<ComponentInterface*>(attr);

    if (cInterface->Init != nullptr)
    {
        // run initialization function if it exists.
        cInterface->Init(this, entity, data);
    }
}

template <>
Game::Position World::GetComponent(Entity entity);
template <>
Game::Orientation World::GetComponent(Entity entity);
template <>
Game::Scale World::GetComponent(Entity entity);

template <>
void World::SetComponent(Entity entity, Game::Position const&);
template <>
void World::SetComponent(Entity entity, Game::Orientation const&);
template <>
void World::SetComponent(Entity entity, Game::Scale const&);

} // namespace Game
