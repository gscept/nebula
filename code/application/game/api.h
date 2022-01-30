#pragma once
//------------------------------------------------------------------------------
/**
    @file   api.h

    The main programming interface for the Game Subsystem.

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "category.h"
#include "entity.h"
#include "memdb/table.h"
#include "memdb/tablesignature.h"
#include "memdb/typeregistry.h"
#include "world.h"
#include "filter.h"
#include "dataset.h"

namespace MemDb
{
class Database;
}

namespace Game
{

//class ProcessorBuilder
//{
//public:
//    ProcessorBuilder() = delete;
//    ProcessorBuilder(Util::StringAtom processorName);
//
//    /// which function to run with the processor
//    ProcessorBuilder& Func(std::function<void> func);
//    
//    /// entities must have these components
//    template<typename ... COMPONENTS>
//    ProcessorBuilder& With();
//
//    /// entities must not have any of these components
//    template<typename ... COMPONENTS>
//    ProcessorBuilder& Excluding();
//
//    /// select on which event the processor is executed
//    ProcessorBuilder& On(Util::StringAtom eventName);
//    
//    /// processor should run async
//    ProcessorBuilder& Async();
//    
//    /// create and register the processor
//    ProcessorHandle Create();
//
//private:
//    Util::StringAtom name;
//    bool async = false;
//    ProcessorFrameCallback func = nullptr;
//};

//------------------------------------------------------------------------------

#define WORLD_DEFAULT uint32_t('DWLD')

//------------------------------------------------------------------------------
//      Types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Maps an entity to a table and instance id
*/
struct EntityMapping
{
    MemDb::TableId table;
    MemDb::Row instance;
};

/// Opaque entity operations buffer
typedef uint32_t OpBuffer;

/// Opaque processor handle
typedef uint32_t ProcessorHandle;


//------------------------------------------------------------------------------
//      Create, Setup and Registration
//------------------------------------------------------------------------------

static const uint32_t MAX_NUM_CATEGORIES = 512;

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
   Specifies special behaviour for a components
*/
enum ComponentFlags : uint32_t
{
    /// regular component
    COMPONENTFLAG_NONE = 0,
    /// managed component. This will delay the deletion of this component by
    /// one frame, allowing managers to clean up externally allocated resources
    COMPONENTFLAG_MANAGED = 1 << 0
};

//------------------------------------------------------------------------------
/**
    Used to create a component.
    
    @note   types must be mem- copyable, and trivially destructible and should
            preferably not define a constructor.
*/
struct ComponentCreateInfo
{
    /// name of the component
    const char* name;
    /// size of the component type in bytes.
    uint32_t byteSize;
    /// a default value for the component type, or NULL if we always want to initialize to 0's
    void const* defaultValue;
    /// component flags
    ComponentFlags flags = ComponentFlags::COMPONENTFLAG_NONE;
};

/// per frame callback for processors
using ProcessorFrameCallback = std::function<void(World*, Dataset)>;

//------------------------------------------------------------------------------
/**
*/
struct ProcessorCreateInfo
{
    /// name of the processor
    Util::StringAtom name;

    /// set if this processor should run as a job.
    /// TODO: this is currently not used
    bool async = false;
    /// filter used for creating the dataset
    Filter filter;

    /// called when attached to world
    //void(*OnActivate)() = nullptr;
    ///// called when removed from world
    //void(*OnDeactivate)() = nullptr;
    ///// called by Game::Server::Start()
    //void(*OnStart)() = nullptr;

    /// called before frame by the game server
    ProcessorFrameCallback OnBeginFrame = nullptr;
    /// called per-frame by the game server
    ProcessorFrameCallback OnFrame = nullptr;
    /// called after frame by the game server
    ProcessorFrameCallback OnEndFrame = nullptr;
    /// called after loading game state
    ProcessorFrameCallback OnLoad = nullptr;
    /// called before saving game state
    ProcessorFrameCallback OnSave = nullptr;
    /// render a debug visualization 
    ProcessorFrameCallback OnRenderDebug = nullptr;
};

//------------------------------------------------------------------------------
/**
*/
struct ComponentDecayBuffer
{
    uint32_t size = 0;
    uint32_t capacity = 0;
    void* buffer = nullptr;
};

//------------------------------------------------------------------------------
//      Entity Operations
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
//      Functions
//------------------------------------------------------------------------------

/// returns a world by hash
World* GetWorld(uint32_t worldHash);
/// Create a new entity
Game::Entity                CreateEntity(World*, EntityCreateInfo const& info);
/// delete entity
void                        DeleteEntity(World*, Game::Entity entity);
/// Check if an entity ID is still valid.
bool                        IsValid(World*, Entity e);
/// Check if an entity is active (has an instance). It might be valid, but inactive just after it has been created.
bool                        IsActive(World*, Entity e);
/// Returns the entity mapping of an entity
EntityMapping               GetEntityMapping(World*, Entity entity);
/// Get instance of entity
MemDb::Row                  GetInstance(World*, Entity entity);
/// add a component to an entity.
template<typename TYPE>
void                        AddComponent(World*, Entity, TYPE* prop);
/// remove a component from an entity
template<typename TYPE>
void                        RemoveComponent(World*, Entity);
/// typed set component
template<typename TYPE>
void                        SetComponent(World*, Game::Entity const entity, ComponentId const component, TYPE value);
/// typed get component
template<typename TYPE>
TYPE                        GetComponent(World*, Game::Entity const entity, ComponentId const component);
/// Check if entity has a specific component. (SLOW!)
bool                        HasComponent(World*, Entity const entity, ComponentId const component);


/// Create an operations buffer
OpBuffer                    CreateOpBuffer(World*);
/// Runs all operations in an operations buffer and clears it. This will invalidate category table views.
void                        Dispatch(OpBuffer buffer);
/// Destroys an operations buffer
void                        DestroyOpBuffer(OpBuffer&);
/// Register a component <-> entity association. Moves entity to a different entity table.
void                        AddOp(OpBuffer buffer, Op::RegisterComponent op);
/// Deregister a component <-> entity association. Moves entity to a different entity table.
void                        AddOp(OpBuffer buffer, Op::DeregisterComponent const& op);
/// Execute an operation directly
void                        Execute(World*, Op::RegisterComponent const& op);
/// Execute an operation directly
void                        Execute(World*, Op::DeregisterComponent const& op);
/// Release all memory allocated by operation buffers
void                        ReleaseAllOps();


/// Destroy a filter
void                        DestroyFilter(Filter);

/// Create a processor
ProcessorHandle             CreateProcessor(ProcessorCreateInfo const& info);

/// Query the entity database using specified filter set. This does NOT wait for resources to be available.
Dataset                     Query(World*, Filter filter);
/// Query a subset of tables using a specified filter set. Modifies the tables array so that it only contains valid tables. This does NOT wait for resources to be available.
Dataset                     Query(World*, Util::Array<MemDb::TableId>& tables, Filter filter);
/// Query a subset of tables in a specific db using a specified filter set. Modifies the tables array so that it only contains valid tables. This does NOT wait for resources to be available.
Dataset                     Query(Ptr<MemDb::Database> const& db, Util::Array<MemDb::TableId>& tables, Filter filter);
/// Recycles all current datasets allocated memory to be reused
void                        ReleaseDatasets();

/// Create a component
ComponentId                  CreateComponent(ComponentCreateInfo const& info);
/// Returns a component id
ComponentId                  GetComponentId(Util::StringAtom name);
/// Returns a blueprint id by name
BlueprintId                 GetBlueprintId(Util::StringAtom name);
/// Returns a template id by name
TemplateId                  GetTemplateId(Util::StringAtom name);
/// Get number of instances in a specific table
SizeT                       GetNumInstances(World*, MemDb::TableId table);
/// retrieve the instance buffer for a specific component in a table
void*                       GetInstanceBuffer(World*, MemDb::TableId const, ComponentId const);

// Following functions are unsafe to use in general scenarios

/// allocate an entity id
Entity                      AllocateEntity(World*);
/// deallocate an entity id. Make sure to deallocate the entity db entry first.
void                        DeallocateEntity(World*, Entity);
/// allocate an instance in the table and map it to the entity
MemDb::Row                  AllocateInstance(World*, Entity, MemDb::TableId);
/// allocate an instance from a blueprint in the table and map it to the entity
MemDb::Row                  AllocateInstance(World*, Entity, BlueprintId);
/// allocate an instance from a template in the table and map it to the entity
MemDb::Row                  AllocateInstance(World*, Entity, TemplateId);
/// deallocate an entitys entry from the worlds database
void                        DeallocateInstance(World*, Entity);
/// deallocate an entry from the worlds database. @note this does not disassociate it with any entity, which might lead to subtle bugs if not handled correctly.
void                        DeallocateInstance(World*, MemDb::TableId, MemDb::Row);
/// migrate an entity to a different table
MemDb::Row                  Migrate(World*, Entity, MemDb::TableId toTable);
/// migrate an array of entities from one table to another. Fills the newInstances array with the new row ids for each entity. @note assumes all entities are associated with the fromTable.
void                        Migrate(World*, Util::Array<Entity> const& entities, MemDb::TableId fromTable, MemDb::TableId toTable, Util::FixedArray<IndexT>& newInstances);
/// register processors to a world
void                        RegisterProcessors(World*, std::initializer_list<ProcessorHandle>);
/// defragment an entity table
void                        Defragment(World*, MemDb::TableId);
/// create an entity table
MemDb::TableId              CreateEntityTable(World* world, CategoryCreateInfo const& info);
/// set the value of an entity
void                        SetComponent(World*, Game::Entity entity, Game::ComponentId component, void* value, uint64_t size);
/// get the decay buffer for a specific component
ComponentDecayBuffer const   GetDecayBuffer(Game::ComponentId component);
/// clear the component decay buffers
void                        ClearDecayBuffers();
/// register an update function that runs for each entity that fulfill the requirements of the function, contains any of the additional inclusive propertoes, and does not have any properties contained in the exclusive set.
/// @deprecated     Will be removed when the new ProcessorBuilder is implemented.
template<typename ... TYPES>
Game::ProcessorHandle       RegisterUpdateFunction(World*, Util::StringAtom name, std::function<void(TYPES...)> func, std::initializer_list<ComponentId> additionalInclusive = {}, std::initializer_list<ComponentId> exclusive = {});


//------------------------------------------------------------------------------
/**
    -- Beginning of template implementations --
*/

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline void
SetComponent(World* world, Game::Entity const entity, ComponentId const component, TYPE value)
{
#if NEBULA_DEBUG
    n_assert2(sizeof(TYPE) == MemDb::TypeRegistry::TypeSize(component), "SetComponent: Provided value's type is not the correct size for the given ComponentId.");
#endif
    EntityMapping mapping = GetEntityMapping(world, entity);
    TYPE* ptr = (TYPE*)GetInstanceBuffer(world, mapping.table, component);
    *(ptr + mapping.instance) = value;
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline TYPE
GetComponent(World* world, Game::Entity const entity, ComponentId const component)
{
#if NEBULA_DEBUG
    n_assert2(sizeof(TYPE) == MemDb::TypeRegistry::TypeSize(component), "GetComponent: Provided value's type is not the correct size for the given ComponentId.");
#endif
    EntityMapping mapping = GetEntityMapping(world, entity);
    TYPE* ptr = (TYPE*)GetInstanceBuffer(world, mapping.table, component);
    return *(ptr + mapping.instance);
}

//------------------------------------------------------------------------------
/**
    Internally used template functions
*/
namespace Internal
{
    template<class TYPE>
    void SetInclusive(Game::FilterBuilder::FilterCreateInfo& filterInfo, size_t const i)
    {
        using UnqualifiedType = typename std::remove_const<typename std::remove_reference<TYPE>::type>::type;

        filterInfo.inclusive[i] = UnqualifiedType::ID();
        filterInfo.access[i] = std::is_const<typename std::remove_reference<TYPE>::type>() ? Game::AccessMode::READ : Game::AccessMode::WRITE;
        filterInfo.numInclusive++;
    }

    template<typename...TYPES, std::size_t...Is>
    void UnrollInclusiveComponents(Game::FilterBuilder::FilterCreateInfo& filterInfo, std::index_sequence<Is...>)
    {
        (SetInclusive<typename std::tuple_element<Is, std::tuple<TYPES...>>::type>(filterInfo, Is), ...);
    }

    template<typename...TYPES, std::size_t...Is>
    void update_expander(std::function<void(TYPES...)> const& func, Game::Dataset::EntityTableView const& view, const IndexT instance, std::index_sequence<Is...>)
    {
        // this is a terribly unreadable line. Here's what it does:
        // it unpacks the the index sequence and TYPES into individual parameters for func
        // because we need to cast void pointers (the view buffers), we need to remove any const and reference qualifiers from the type.
        func((*(((typename std::remove_const<typename std::remove_reference<TYPES>::type>::type*)view.buffers[Is]) + instance))...);
    }
}

//------------------------------------------------------------------------------
/**
    @todo   ability to register different event types
*/
template<typename ...TYPES>
inline Game::ProcessorHandle
RegisterUpdateFunction(World* world, Util::StringAtom name, std::function<void(TYPES...)> func, std::initializer_list<ComponentId> additionalInclusive, std::initializer_list<ComponentId> exclusive)
{
    n_assert(exclusive.size() < 0xFF);
    n_assert(additionalInclusive.size() < 0xFF);

    ProcessorFrameCallback processor = [func](World* world, Game::Dataset data) {
        for (int v = 0; v < data.numViews; v++)
        {
            Game::Dataset::EntityTableView const& view = data.views[v];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                Internal::update_expander<TYPES...>(func, view, i, std::make_index_sequence<sizeof...(TYPES)>());
            }
        }
    };

    Game::FilterBuilder::FilterCreateInfo filterInfo;
    Internal::UnrollInclusiveComponents<TYPES...>(filterInfo, std::make_index_sequence<sizeof...(TYPES)>());
    for (int i = 0; i < (int)additionalInclusive.size(); i++)
        filterInfo.inclusive[filterInfo.numInclusive + i] = *(additionalInclusive.begin() + i);
    filterInfo.numInclusive = filterInfo.numInclusive + (uint8_t)additionalInclusive.size();
    for (int i = 0; i < exclusive.size(); i++)
        filterInfo.exclusive[i] = *(exclusive.begin() + i);
    filterInfo.numExclusive = (uint8_t)exclusive.size();
    Game::Filter filter = Game::FilterBuilder::CreateFilter(filterInfo);
    Game::ProcessorCreateInfo processorInfo;
    processorInfo.async = false;
    processorInfo.filter = filter;
    processorInfo.name = name;
    processorInfo.OnBeginFrame = processor;

    Game::ProcessorHandle pHandle = Game::CreateProcessor(processorInfo);
    return pHandle;
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline void
AddComponent(World* world, Entity entity, TYPE* value)
{
    //n_assert(!state.asyncProcessing);
    Op::RegisterComponent op;
    op.entity = entity;
    op.component = TYPE::ID();
    op.value = (void*)value;
    AddOp(WorldGetScratchOpBuffer(world), op);
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline void
RemoveComponent(World* world, Entity entity)
{
    //n_assert(!state.asyncProcessing);
    Op::DeregisterComponent op;
    op.entity = entity;
    op.component = TYPE::ID();
    AddOp(WorldGetScratchOpBuffer(world), op);
}

} // namespace Game
