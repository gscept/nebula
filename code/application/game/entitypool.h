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
#include "memdb/table.h"

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

    /// allocate a new id, returns false if the entity id was reused
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
        MemDb::TableId table;
        MemDb::Row row;
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
    Util::HashTable<MemDb::TableId, MemDb::TableId> categoryDecayMap;
    /// maps from blueprint to a category that has the same signature
    Util::HashTable<BlueprintId, MemDb::TableId> blueprintCatMap;
    ///
    Util::Queue<AllocateInstanceCommand> allocQueue;
    ///
    Util::Queue<DeallocInstanceCommand> deallocQueue;

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
};

} // namespace Game
