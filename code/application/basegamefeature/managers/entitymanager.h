#pragma once
//------------------------------------------------------------------------------
/**
    @class  Game::EntityManager

    Keeps track of all existing entites.

    @copyright
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

/// typed get property method
template<typename TYPE>
TYPE GetProperty(Game::Entity const entity, PropertyId const pid);

/// Returns the world db
Ptr<MemDb::Database> GetWorldDatabase();

/// Generation pool
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

    uint8_t worldId;
    /// array containing generation value for every index
    Util::Array<uint8_t> generations;
    /// stores freed indices
    Util::Queue<uint32_t> freeIds;
};

struct World
{
    /// used to allocate entity ids for this world
    EntityPool pool;
    /// Number of entities alive
    SizeT numEntities;
    /// maps entity index to category+instanceid pair
    Util::Array<EntityMapping> entityMap;
    /// contains all entity instances
    Ptr<MemDb::Database> db;
    /// the world identifier
    uint8_t id;
    /// name of the world
    Util::StringAtom name;

    /*
        // TODO: Move entity management and world db management to the game server
        WorldId worldId = GameServer::CreateWorld("Overworld");
        
        // order of processors will set up dependencies for the job system.
        // processors are binned to different events depending on which callbacks are set.
        // this can be called multiple times for the same world as well.
        GameServer::RegisterProcessors(worldId, {
            "GraphicsManager.CreateModels",
            "GraphicsManager.DestroyModels",
            "GraphicsManager.UpdateModelTransforms",
            "CameraManager.UpdateCameraWorldTransformed",
            "PhysicsManager.CreateActors",
            ...
        });

        // Managers shouldn't need to be registered per world - They should be globally
        // managing things, mostly just connecting different subsystems to the entity system and
        // setting up processors.

        // TODO: What do we do with categories?
        //       Should we just keep a category list per world, or should the categories
        //       contain an instance table per world?
    */
    /*
    struct ProcessorCallbackInfo
    {
        ProcessorHandle handle;
        Filter filter;
        ProcessorFrameCallback func;
    };

    struct ProcessorInfo
    {
        Util::StringAtom name;
        /// set if this processor should run as a job
        bool async = false;
        /// called when removed from game server
        void(*OnDeactivate)() = nullptr;
    };

    Util::Array<ProcessorCallbackInfo> onBeginFrameCallbacks;
    Util::Array<ProcessorCallbackInfo> onFrameCallbacks;
    Util::Array<ProcessorCallbackInfo> onEndFrameCallbacks;
    Util::Array<ProcessorCallbackInfo> onLoadCallbacks;
    Util::Array<ProcessorCallbackInfo> onSaveCallbacks;
    Util::Array<ProcessorInfo> processors;
    Ids::IdGenerationPool processorHandlePool;
    */
};

//------------------------------------------------------------------------------
/**
    @class  Game::EntityManager
    
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

        World world;

        //Ptr<MemDb::Database> worldDatabase;
        //Ids::IdGenerationPool pool;
        //SizeT numEntities;

        /// Contains all templates
        Ptr<MemDb::Database> templateDatabase;

        Util::Queue<AllocateInstanceCommand> allocQueue;
        Util::Queue<DeallocInstanceCommand> deallocQueue;

        /// categories
        Util::Array<Category> categoryArray;
        /// maps from catagory hash to category id
        Util::HashTable<CategoryHash, CategoryId> catIndexMap;

        /// maps from entity index to category and instance id
        //Util::Array<EntityMapping> entityMap;

        /// quick access to the Owner property id
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
    n_assert(cid != CategoryId::Invalid());
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
    Ptr<MemDb::Database> db = EntityManager::Singleton->state.world.db;
    auto cid = db->GetColumnId(cat.instanceTable, pid);

#if NEBULA_DEBUG
    n_assert_fmt(cid != MemDb::ColumnIndex::Invalid(), "SetProperty: Entity does not have property with id '%i'!\n", cid.id);
    n_assert2(sizeof(TYPE) == MemDb::TypeRegistry::TypeSize(pid), "SetProperty: Provided value's type is not the correct size for the given PropertyId.");
#endif

    TYPE* ptr = (TYPE*)db->GetValuePointer(cat.instanceTable, cid, mapping.instance.id);
    *ptr = value;
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
TYPE
GetProperty(Game::Entity const entity, PropertyId const pid)
{
    EntityMapping mapping = GetEntityMapping(entity);
    Category const& cat = EntityManager::Singleton->GetCategory(mapping.category);
    Ptr<MemDb::Database> db = EntityManager::Singleton->state.world.db;
    auto cid = db->GetColumnId(cat.instanceTable, pid);

#if NEBULA_DEBUG
    n_assert_fmt(cid != MemDb::ColumnIndex::Invalid(), "GetProperty: Entity does not have type with PropertyId '%i'!\n", cid.id);
    n_assert2(sizeof(TYPE) == MemDb::TypeRegistry::TypeSize(pid), "GetProperty: Provided value's type is not the correct size for the given PropertyId.");
#endif

    TYPE* ptr = (TYPE*)db->GetValuePointer(cat.instanceTable, cid, mapping.instance.id);
    return *ptr;
}

//-------------------------
} // namespace Game
