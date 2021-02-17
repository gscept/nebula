#pragma once
//------------------------------------------------------------------------------
/**
    @file entitypool.h

    @copyright
    (C) 2021 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "api.h"
#include "category.h"
#include "util/queue.h"
#include "memdb/database.h"

namespace Game
{

//------------------------------------------------------------------------------
/**
    Generation pool
*/
class EntityPool
{
public:
    // default constructor
    EntityPool();

    /// allocate a new id, returns whether or not the id was reused or new
    bool Allocate(Entity& e);
    /// remove an id
    void Deallocate(Entity e);
    /// check if valid
    bool IsValid(Entity e) const;

    /// array containing generation value for every index
    Util::Array<uint16_t> generations;
    /// stores freed indices
    Util::Queue<uint32_t> freeIds;
};

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
        CategoryId category;
        InstanceId instance;
    };

    /// used to allocate entity ids for this world
    EntityPool pool;
    /// Number of entities alive
    SizeT numEntities;
    /// maps entity index to category+instanceid pair
    Util::Array<Game::EntityMapping> entityMap;
    /// contains all entity instances
    Ptr<MemDb::Database> db;
    /// name of the world
    Util::StringAtom name;
    /// when an entity in a category within this table is destroyed, it is moved to the decay table.
    Util::HashTable<CategoryId, MemDb::TableId> categoryDecayMap;
    /// maps from blueprint to a category that has the same signature
    Util::HashTable<BlueprintId, CategoryId> blueprintCatMap;
    ///
    Util::Queue<AllocateInstanceCommand> allocQueue;
    ///
    Util::Queue<DeallocInstanceCommand> deallocQueue;

    /// creates a category
    CategoryId CreateCategory(CategoryCreateInfo const& info);

    /// returns the number of existing categories
    SizeT const GetNumCategories() const;

    /// allocate instance for entity in category instance table
    InstanceId AllocateInstance(Entity entity, CategoryId category);

    /// allocate instance for entity in blueprints category instance table
    InstanceId AllocateInstance(Entity entity, BlueprintId blueprint);

    /// allocate instance for entity in blueprint instance table by copying template
    InstanceId AllocateInstance(Entity entity, TemplateId templateId);

    /// deallocated and recycle instance in category instance table
    void DeallocateInstance(Entity entity);
    /// deallocated and recycle instance in category instance table
    void DeallocateInstance(CategoryId category, InstanceId instance);

    /// migrate an instance from one category to another
    InstanceId Migrate(Entity entity, CategoryId newCategory);
    /// migrate an n instances from one category to another
    void Migrate(Util::Array<Entity> const& entities, CategoryId fromCategory, CategoryId newCategory, Util::FixedArray<IndexT>& newInstances);

    /// register a processor that processes entities in this world
    void RegisterProcessor(std::initializer_list<ProcessorHandle> handles);

    /// add the table to any callback-caches that accepts it
    void CacheTable(MemDb::TableId tid, MemDb::TableSignature signature);

    /// reset and pre filter the callbacks
    void Prefilter();

    /// call this if you need to defragment the category instance table
    void DefragmentCategoryInstances(CategoryId cat);

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
};

} // namespace Game
