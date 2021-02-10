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
    Util::Array<uint8_t> generations;
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
        Game::Entity entity;
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
    /// categories
    Util::Array<Category> categoryArray;
    /// maps from hash to category id
    Util::HashTable<CategoryHash, CategoryId> catIndexMap;
    /// maps from blueprint to a category that has the same signature
    Util::HashTable<BlueprintId, CategoryId> blueprintCatMap;
    ///
    Util::Queue<AllocateInstanceCommand> allocQueue;
    ///
    Util::Queue<DeallocInstanceCommand> deallocQueue;

    /// creates a category
    CategoryId CreateCategory(CategoryCreateInfo const& info);

    /// returns a category by id. asserts if category does not exist
    Category const& GetCategory(CategoryId cid) const;

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

    /// migrate an instance from one category to another
    InstanceId Migrate(Entity entity, CategoryId newCategory);

    /// migrate an n instances from one category to another
    void Migrate(Util::Array<Entity> const& entities, CategoryId fromCategory, CategoryId newCategory, Util::FixedArray<IndexT>& newInstances);
};

//------------------------------------------------------------------------------
/**
*/
inline Category const&
World::GetCategory(CategoryId cid) const
{
    n_assert(cid != CategoryId::Invalid());
    return this->categoryArray[cid.id];
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT const
World::GetNumCategories() const
{
    return this->categoryArray.Size();
}

} // namespace Game
