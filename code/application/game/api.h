#pragma once
//------------------------------------------------------------------------------
/**
    @file   api.h

    The main programming interface for the Game Subsystem.

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "category.h"
#include "memdb/tablesignature.h"

namespace Game
{

//------------------------------------------------------------------------------
//      Forward declarations
//------------------------------------------------------------------------------
class World;

#define WORLD_DEFAULT uint32_t('DWLD')

//------------------------------------------------------------------------------
//      Types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Maps an entity to a category and instance id
*/
struct EntityMapping
{
    MemDb::TableId category;
    MemDb::Row instance;
};

//------------------------------------------------------------------------------
/**
    A dataset that contains views into category tables. These are created by
    querying the world database.
*/
struct Dataset
{
    static const uint32_t MAX_PROPERTY_BUFFERS = 64;

    /// a view into a category table
    struct CategoryTableView
    {
        /// category identifier
        MemDb::TableId cid;
        /// number of instances in view
        uint32_t numInstances = 0;
        /// property buffers. @note Can be NULL if a queried property is a flag
        void* buffers[MAX_PROPERTY_BUFFERS];
    };

    /// number of views in views array
    uint32_t numViews = 0;
    /// views into the tables
    CategoryTableView* views = nullptr;
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
    /// blueprint to instantiate from
    BlueprintId blueprint = BlueprintId::Invalid();
    /// template to instantiate. If this is set, the blueprint does not have to be set.
    TemplateId templateId = TemplateId::Invalid();
    /// set if the entity should be instantiated immediately or deferred until end of frame.
    bool immediate = false;
};

//------------------------------------------------------------------------------
/**
*/
struct WorldCreateInfo
{
    uint32_t hash;
};

//------------------------------------------------------------------------------
/**
*/
enum AccessMode
{
    READ,
    WRITE
};

/// Opaque filter identifier.
typedef uint32_t Filter;

//------------------------------------------------------------------------------
/**
*/
struct FilterCreateInfo
{
    static const uint32_t MAX_EXCLUSIVE_PROPERTIES = 32;

    /// number of properties in the inclusive set
    uint8_t numInclusive = 0;
    /// inclusive set
    PropertyId inclusive[Dataset::MAX_PROPERTY_BUFFERS];
    /// how we indend to access the properties
    AccessMode access[Dataset::MAX_PROPERTY_BUFFERS];
    /// number of properties in the exclusive set
    uint8_t numExclusive = 0;
    /// exclusive set
    PropertyId exclusive[MAX_EXCLUSIVE_PROPERTIES];
};

//------------------------------------------------------------------------------
/**
   Specifies special behaviour for a property
*/
enum PropertyFlags : uint32_t
{
    /// regular property
    PROPERTYFLAG_NONE = 0,
    /// managed property. This will delay the deletion of this property by
    /// one frame, allowing managers to clean up externally allocated resources
    PROPERTYFLAG_MANAGED = 1 << 0
};

//------------------------------------------------------------------------------
/**
    Used to create a property.
    
    @note   types must be mem- copyable, and trivially destructible and should
            preferably not define a constructor.
*/
struct PropertyCreateInfo
{
    /// name of the property
    const char* name;
    /// size of the property type in bytes.
    uint32_t byteSize;
    /// a default value for the property type, or NULL if we always want to initialize to 0's
    void const* defaultValue;
    /// property flags
    PropertyFlags flags = PropertyFlags::PROPERTYFLAG_NONE;
};

/// per frame callback for processors
typedef void(*ProcessorFrameCallback)(World*, Dataset);

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

typedef MemDb::TableSignature InclusiveTableMask;
typedef MemDb::TableSignature ExclusiveTableMask;

//------------------------------------------------------------------------------
//      Entity Operations
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
namespace Op
{

//------------------------------------------------------------------------------
/**
*/
struct RegisterProperty
{
    Entity entity;
    PropertyId pid;
    void const* value = nullptr;
};

//------------------------------------------------------------------------------
/**
*/
struct DeregisterProperty
{
    Entity entity;
    PropertyId pid;
};

} // namespace Op
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
//      Functions
//------------------------------------------------------------------------------

World* CreateWorld(WorldCreateInfo const& info);
/// returns a world by hash
World* GetWorld(uint32_t worldHash);
/// Returns the world db
Ptr<MemDb::Database>        GetWorldDatabase(World*);
/// Create a new entity
Game::Entity                CreateEntity(World*, EntityCreateInfo const& info);
/// delete entity
void                        DeleteEntity(World*, Game::Entity entity);
/// typed set a property method.
template<typename TYPE>
void                        SetProperty(World*, Game::Entity const entity, PropertyId const pid, TYPE value);
/// typed get property method
template<typename TYPE>
TYPE                        GetProperty(World*, Game::Entity const entity, PropertyId const pid);
/// Create an operations buffer
OpBuffer                    CreateOpBuffer(World*);
/// Runs all operations in an operations buffer and clears it. This will invalidate category table views.
void                        Dispatch(OpBuffer& buffer);
/// Register a property <-> entity association. Moves entity to a different category table.
void                        AddOp(OpBuffer buffer, Op::RegisterProperty op);
/// Deregister a property <-> entity association. Moves entity to a different category table.
void                        AddOp(OpBuffer buffer, Op::DeregisterProperty const& op);
/// Execute an operation directly
void                        Execute(World*, Op::RegisterProperty const& op);
/// Execute an operation directly
void                        Execute(World*, Op::DeregisterProperty const& op);
/// Release all memory allocated by operation buffers
void                        ReleaseAllOps();
/// Create a filter
Filter                      CreateFilter(FilterCreateInfo const& info);
/// Destroy a filter
void                        DestroyFilter(Filter);
/// Create a processor
ProcessorHandle             CreateProcessor(ProcessorCreateInfo const& info);
/// Recycles all current datasets allocated memory to be reused
void                        ReleaseDatasets();
/// Query the entity database using specified filter set. This does NOT wait for resources to be available.
Dataset                     Query(World*, Filter filter);
/// Query a subset of tables using a specified filter set. Modifies the tables array so that it only contains valid tables. This does NOT wait for resources to be available.
Dataset                     Query(World*, Util::Array<MemDb::TableId>& tables, Filter filter);
/// Query a subset of tables in a specific db using a specified filter set. Modifies the tables array so that it only contains valid tables. This does NOT wait for resources to be available.
Dataset                     Query(Ptr<MemDb::Database> const& db, Util::Array<MemDb::TableId>& tables, Filter filter);
/// Get instance of entity
MemDb::Row                  GetInstance(World*, Entity entity);
/// Check if an entity ID is still valid.
bool                        IsValid(World*, Entity e);
/// Check if an entity is active (has an instance). It might be valid, but inactive just after it has been created.
bool                        IsActive(World*, Entity e);
/// Returns the entity mapping of an entity
EntityMapping               GetEntityMapping(World*, Entity entity);
/// Create a property
PropertyId                  CreateProperty(PropertyCreateInfo const& info);
/// Returns a property id
PropertyId                  GetPropertyId(Util::StringAtom name);
/// Check if entity has a specific property. (SLOW!)
bool                        HasProperty(World*, Entity const entity, PropertyId const pid);
/// Returns a blueprint id by name
BlueprintId                 GetBlueprintId(Util::StringAtom name);
/// Returns a template id by name
TemplateId                  GetTemplateId(Util::StringAtom name);
/// Get number of instances in a specific category
SizeT                       GetNumInstances(World*, MemDb::TableId table);
/// retrieve the instance buffer for a specific property in a category
void*                       GetInstanceBuffer(World*, MemDb::TableId const, PropertyId const);
/// retrieve the inclusive table mask
InclusiveTableMask const&   GetInclusiveTableMask(Filter);
/// retrieve the exclusive table mask
ExclusiveTableMask const&   GetExclusiveTableMask(Filter);
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
/// prefilter the database for all processors
void                        PrefilterProcessors(World*);
/// defragment an entity table
void                        Defragment(World*, MemDb::TableId);
/// create an entity table
MemDb::TableId              CreateEntityTable(World* world, CategoryCreateInfo const& info);
/// set the value of an entity
void                        SetProperty(World*, Game::Entity entity, Game::PropertyId pid, void* value, uint64_t size);


//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline void
SetProperty(World* world, Game::Entity const entity, PropertyId const pid, TYPE value)
{
#if NEBULA_DEBUG
    n_assert2(sizeof(TYPE) == MemDb::TypeRegistry::TypeSize(pid), "SetProperty: Provided value's type is not the correct size for the given PropertyId.");
#endif
    EntityMapping mapping = GetEntityMapping(world, entity);
    TYPE* ptr = (TYPE*)GetInstanceBuffer(world, mapping.category, pid);
    *(ptr + mapping.instance) = value;
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline TYPE
GetProperty(World* world, Game::Entity const entity, PropertyId const pid)
{
#if NEBULA_DEBUG
    n_assert2(sizeof(TYPE) == MemDb::TypeRegistry::TypeSize(pid), "GetProperty: Provided value's type is not the correct size for the given PropertyId.");
#endif
    EntityMapping mapping = GetEntityMapping(world, entity);
    TYPE* ptr = (TYPE*)GetInstanceBuffer(world, mapping.category, pid);
    return *(ptr + mapping.instance);
}

} // namespace Game
