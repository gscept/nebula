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
#include "processor.h"

namespace MemDb
{
class Database;
}

namespace Game
{

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
/// typed set component
template<typename TYPE>
void                        SetComponent(World*, Game::Entity const entity, TYPE value);
/// typed get component
template<typename TYPE>
TYPE                        GetComponent(World*, Game::Entity const entity, ComponentId const component);
/// typed get component
template<typename TYPE>
TYPE                        GetComponent(World*, Game::Entity const entity);
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
inline void
SetComponent(World* world, Game::Entity const entity, TYPE value)
{
#if NEBULA_DEBUG
    n_assert2(sizeof(TYPE) == MemDb::TypeRegistry::TypeSize(TYPE::ID()), "SetComponent: Provided value's type is not the correct size for the given ComponentId.");
#endif
    EntityMapping mapping = GetEntityMapping(world, entity);
    TYPE* ptr = (TYPE*)GetInstanceBuffer(world, mapping.table, TYPE::ID());
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
*/
template<typename TYPE>
inline TYPE
GetComponent(World* world, Game::Entity const entity)
{
#if NEBULA_DEBUG
    n_assert2(sizeof(TYPE) == MemDb::TypeRegistry::TypeSize(TYPE::ID()), "GetComponent: Provided value's type is not the correct size for the given ComponentId.");
#endif
    EntityMapping mapping = GetEntityMapping(world, entity);
    TYPE* ptr = (TYPE*)GetInstanceBuffer(world, mapping.table, TYPE::ID());
    return *(ptr + mapping.instance);
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
