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
#include "memory/arenaallocator.h"

namespace MemDb { class Database; }

namespace Game
{

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

template<typename COMPONENT_TYPE>
struct ComponentRegisterInfo
{
    using OnInitFunc = void (*)(Game::World*, Game::Entity, COMPONENT_TYPE*);

    /// Set to true if the component should end up in the decay buffer before being completely destroyed.
    bool decay = false;
    /// initialization function to run for the component, or nullptr if not needed.
    OnInitFunc OnInit = nullptr;
};

//------------------------------------------------------------------------------
/**
*/
class World
{
public:
    ~World();

    /// Register a component type
    template <typename COMPONENT_TYPE>
    ComponentId RegisterType(ComponentRegisterInfo<COMPONENT_TYPE> info = {});

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
    /// Check if entity has a specific component.
    template <typename TYPE>
    bool HasComponent(Game::Entity const entity);
    /// Get instance of entity
    MemDb::RowId GetInstance(Entity entity);
    /// Remove a component from an entity
    template <typename TYPE>
    void RemoveComponent(Entity);
    /// Set the value of an entitys component
    template <typename TYPE>
    void SetComponent(Entity entity, TYPE value);
    /// Get an entitys component
    template <typename TYPE>
    TYPE GetComponent(Entity entity);
    

    // -----------
    Memory::ArenaAllocator<4096_KB> componentStageAllocator;
    
    // Create a component.
    // These are constructed directly if it has an OnStage delegate.
    // This queues the component in a command buffer to be added later
    template <typename TYPE>
    TYPE* AddComponent(Entity entity);

    /// Get a decay buffer for the given component
    ComponentDecayBuffer const GetDecayBuffer(Game::ComponentId component);
    
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
    
    /// These are registered to the attribute registry so that we can add more functionality to attributes
    class ComponentInterface : public MemDb::Attribute
    {
    public:
        /// construct from template type, with default value.
        template <typename T>
        explicit ComponentInterface(Util::StringAtom name, T const& defaultValue, uint32_t flags)
            : Attribute(name, defaultValue, flags)
        {
            // empty
        }

        ComponentInitFunc Init = nullptr;
    };

    struct AllocateInstanceCommand
    {
        Game::Entity entity;
        TemplateId tid;
    };
    struct DeallocInstanceCommand
    {
        Game::Entity entity;
    };
    struct AddStagedComponentCommand
    {
        Game::Entity entity;
        ComponentId componentId;
        SizeT dataSize;
        void* data;
    };
    struct RemoveComponentCommand
    {
        Entity entity;
        ComponentId componentId;
    };


    // Only the game server should create worlds
    World(uint32_t hash);

    // These functions are called from game server
    void Start();
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

    /// dispatches all staged components to be added to entities
    void ExecuteAddComponentCommands();
    void ExecuteRemoveComponentCommands();

    void AddStagedComponentsToEntity(Entity entity, AddStagedComponentCommand* cmds, SizeT numCmds);
    void RemoveComponentsFromEntity(Entity entity, RemoveComponentCommand* cmds, SizeT numCmds);

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

    void InitializeAllComponents(Entity entity, MemDb::TableId tableId, MemDb::RowId row);

    /// Defragment an entity table
    void Defragment(MemDb::TableId cat);

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
    ///
    Util::Array<AddStagedComponentCommand> addStagedQueue;
    ///
    Util::Array<RemoveComponentCommand> removeComponentQueue;

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

template <typename COMPONENT_TYPE>
ComponentId 
World::RegisterType(ComponentRegisterInfo<COMPONENT_TYPE> info)
{
    uint32_t componentFlags = 0;
    componentFlags |= (uint32_t)COMPONENTFLAG_DECAY * (uint32_t)info.decay;

    ComponentInterface* cInterface = new ComponentInterface(
        COMPONENT_TYPE::Traits::name,
        COMPONENT_TYPE(),
        componentFlags
    );
    cInterface->Init = reinterpret_cast<ComponentInitFunc>(info.OnInit);
    Game::ComponentId const cid = MemDb::AttributeRegistry::Register<COMPONENT_TYPE>(cInterface);
    Game::ComponentSerialization::Register<COMPONENT_TYPE>(cid);
    Game::ComponentInspection::Register(cid, &Game::ComponentDrawFuncT<COMPONENT_TYPE>);
    return cid;
}

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
World::RemoveComponent(Entity entity)
{
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
World::HasComponent(Game::Entity const entity)
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
    Game::ComponentId id = Game::GetComponentId<TYPE>();
#if _DEBUG
    n_assert(MemDb::AttributeRegistry::TypeSize(id) == sizeof(TYPE));
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

} // namespace Game
