#pragma once
//------------------------------------------------------------------------------
/**
    @file   api.h

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
*/
struct Dataset
{
    static const uint32_t MAX_PROPERTY_BUFFERS = 64;

    /// a view into a category table
    struct CategoryTableView
    {
        CategoryId cid;
        uint32_t numInstances = 0;
        void* buffers[MAX_PROPERTY_BUFFERS];
    };

    /// number of views in tablesViews array
    uint32_t numViews;
    /// views into the tables
    CategoryTableView* views;
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
    uint8_t numInclusive;
    /// inclusive set
    PropertyId inclusive[Dataset::MAX_PROPERTY_BUFFERS];
    /// how we indend to access the properties
    AccessMode access[Dataset::MAX_PROPERTY_BUFFERS];
    /// number of properties in the exclusive set
    uint8_t numExclusive;
    /// exclusive set
    PropertyId exclusive[MAX_EXCLUSIVE_PROPERTIES];
};

//------------------------------------------------------------------------------
/**
    @note   types must be mem- copyable, and trivially destructible and should
            preferably not define a constructor.
*/
struct _PropertyInfo
{
    /// name of the property
    const char* name;
    /// property descriptor as a four character code.
    // uint32_t descriptor;
    /// size of the property type in bytes
    uint32_t byteSize;
    /// a default value for the property type, or NULL if we always want to initialize to 0's
    void* defaultValue;
};

typedef void(*ProcessorFrameCallback)(Dataset);

//------------------------------------------------------------------------------
/**
*/
struct ProcessorCreateInfo
{
    Util::StringAtom name;

    /// set if this processor should run as a job
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
    void const* value;
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

} // namespace Game
