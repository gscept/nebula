#pragma once
//------------------------------------------------------------------------------
/**
    @class	Game::EntityManager

    Keeps track of all existing entites.

    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "ids/idgenerationpool.h"
#include "game/entity.h"
#include "game/manager.h"
#include "util/delegate.h"
#include "game/category.h"
#include "memdb/database.h"
#include "basegamefeature/properties/owner.h"
#include "game/api.h"
#include "util/set.h"

namespace Game
{

/// Create a new entity
Game::Entity CreateEntity(EntityCreateInfo const& info);

/// delete entity
void DeleteEntity(Game::Entity entity);

/// typed set a property method.
template<typename TYPE>
void SetProperty(Game::Entity const entity, PropertyId const pid, TYPE value);

/// Returns the world db
Ptr<MemDb::Database> GetWorldDatabase();

//------------------------------------------------------------------------------
/**
    @class	Game::EntityManager
    
    Contains state with categories and the world database.

    Generally, you won't need to access any of the methods or
    variables within this manager directly.

    The entity manager handles all entity categories in the game.
    Categories are collections of properties, arranged as a table where each row
    is an instance, mapped to an entity.
*/
class EntityManager
{
    __DeclareSingleton(EntityManager);
public:
    /// retrieve the api
    static ManagerAPI Create();

    /// destroy entity manager
    static void Destroy();

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

    // Don't modify state without knowing what you're doing!
    struct State
    {
        struct AllocateInstanceCommand
        {
            Game::Entity entity;
            TemplateId tid;
        };

        struct DeallocInstanceCommand
        {
            Game::Entity entity;
        };

        /// Generation pool
        Ids::IdGenerationPool pool;

        /// Number of entities alive
        SizeT numEntities;

        /// Contains the entire world database
        Ptr<MemDb::Database> worldDatabase;

        /// Contains all templates
        Ptr<MemDb::Database> templateDatabase;

        Util::Queue<AllocateInstanceCommand> allocQueue;
        Util::Queue<DeallocInstanceCommand> deallocQueue;
        /// a set that contains all categories that has entities in their managed property tables.
        Util::Set<CategoryId> managedCleanupSet;

        // - Categories -
        Util::Array<Category> categoryArray;
        Util::HashTable<CategoryHash, CategoryId> catIndexMap;

        Util::Array<EntityMapping> entityMap;

        PropertyId ownerId;
    } state;

private:
    /// constructor
    EntityManager();
    /// destructor
    ~EntityManager();
};

//------------------------------------------------------------------------------
/**
*/
inline Category const&
EntityManager::GetCategory(CategoryId cid) const
{
    return this->state.categoryArray[cid.id];
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT const
EntityManager::GetNumCategories() const
{
    return this->state.categoryArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
void
SetProperty(Game::Entity const entity, PropertyId const pid, TYPE value)
{
    EntityMapping mapping = GetEntityMapping(entity);
    Category const& cat = EntityManager::Singleton->GetCategory(mapping.category);
    Ptr<MemDb::Database> db = EntityManager::Singleton->state.worldDatabase;
    auto cid = db->GetColumnId(cat.instanceTable, pid);

#if NEBULA_DEBUG
    n_assert2(sizeof(TYPE) == MemDb::TypeRegistry::TypeSize(pid), "SetProperty: Provided value's type is not the correct size for the given PropertyId.");
#endif

    TYPE* ptr = (TYPE*)db->GetValuePointer(cat.instanceTable, cid, mapping.instance.id);
    *ptr = value;
}

//-------------------------
} // namespace Game
