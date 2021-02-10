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

namespace Game
{

//------------------------------------------------------------------------------
//      Types
//------------------------------------------------------------------------------

//------------------------------------------------------------------------------
/**
    Maps an entity to a category and instance id
*/
struct EntityMapping
{
    CategoryId category;
    InstanceId instance;
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
        CategoryId cid;
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
typedef void(*ProcessorFrameCallback)(Dataset);

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

    /// called when attached to game server
    void(*OnActivate)() = nullptr;
    /// called when removed from game server
    void(*OnDeactivate)() = nullptr;
    /// called by Game::Server::Start()
    void(*OnStart)() = nullptr;

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
struct WorldCreateInfo
{
    /// name of the world
    Util::StringAtom name;
    /// which processors should be attached to the world
    Util::Array<Util::StringAtom> processors;
};

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

/// Returns the world db
Ptr<MemDb::Database> GetWorldDatabase();

/// Create a new entity
Game::Entity CreateEntity(EntityCreateInfo const& info);

/// delete entity
void DeleteEntity(Game::Entity entity);

/// typed set a property method.
template<typename TYPE>
void SetProperty(Game::Entity const entity, PropertyId const pid, TYPE value);

/// typed get property method
template<typename TYPE>
TYPE GetProperty(Game::Entity const entity, PropertyId const pid);

/// Create an operations buffer
OpBuffer CreateOpBuffer();

/// Runs all operations in an operations buffer and clears it. This will invalidate category table views.
void Dispatch(OpBuffer& buffer);

/// Register a property <-> entity association. Moves entity to a different category table.
void AddOp(OpBuffer buffer, Op::RegisterProperty op);

/// Deregister a property <-> entity association. Moves entity to a different category table.
void AddOp(OpBuffer buffer, Op::DeregisterProperty const& op);

/// Execute an operation directly
void Execute(Op::RegisterProperty const& op);

/// Execute an operation directly
void Execute(Op::DeregisterProperty const& op);

/// Release all memory allocated by operation buffers
void ReleaseAllOps();

/// Create a filter
Filter CreateFilter(FilterCreateInfo const& info);

/// Destroy a filter
void DestroyFilter(Filter);

/// Create a processor
ProcessorHandle CreateProcessor(ProcessorCreateInfo const& info);

/// Recycles all current datasets allocated memory to be reused
void ReleaseDatasets();

/// Query the entity database using specified filter set. This does NOT wait for resources to be available.
Dataset Query(Filter filter);

/// Get instanceid of entity
InstanceId GetInstanceId(Entity entity);

/// Check if an entity ID is still valid.
bool IsValid(Entity e);

/// Check if an entity is active (has an instance). It might be valid, but inactive just after it has been created.
bool IsActive(Entity e);

/// Returns number of active entities
uint GetNumEntities();

/// Returns the entity mapping of an entity
EntityMapping GetEntityMapping(Entity entity);

/// Create a property
PropertyId CreateProperty(PropertyCreateInfo const& info);

/// Returns a property id
PropertyId GetPropertyId(Util::StringAtom name);

/// Check if entity has a specific property. (SLOW!)
bool HasProperty(Entity const entity, PropertyId const pid);

/// Returns a blueprint id by name
BlueprintId GetBlueprintId(Util::StringAtom name);

/// Returns a template id by name
TemplateId GetTemplateId(Util::StringAtom name);

/// Get number of instances in a specific category
SizeT GetNumInstances(CategoryId category);

/// retrieve the instance buffer for a specific property in a category
void* GetInstanceBuffer(CategoryId const category, PropertyId const pid);

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline void
SetProperty(Game::Entity const entity, PropertyId const pid, TYPE value)
{
#if NEBULA_DEBUG
    n_assert2(sizeof(TYPE) == MemDb::TypeRegistry::TypeSize(pid), "SetProperty: Provided value's type is not the correct size for the given PropertyId.");
#endif
    EntityMapping mapping = GetEntityMapping(entity);
    TYPE* ptr = (TYPE*)GetInstanceBuffer(mapping.category, pid);
    *(ptr + mapping.instance.id) = value;
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline TYPE
GetProperty(Game::Entity const entity, PropertyId const pid)
{
#if NEBULA_DEBUG
    n_assert2(sizeof(TYPE) == MemDb::TypeRegistry::TypeSize(pid), "GetProperty: Provided value's type is not the correct size for the given PropertyId.");
#endif
    EntityMapping mapping = GetEntityMapping(entity);
    TYPE* ptr = (TYPE*)GetInstanceBuffer(mapping.category, pid);
    return *(ptr + mapping.instance.id);
}

} // namespace Game
