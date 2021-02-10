//------------------------------------------------------------------------------
//  @file entitypool.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "entitypool.h"
#include "gameserver.h"
#include "basegamefeature/managers/blueprintmanager.h"

namespace Game
{

//------------------------------------------------------------------------------
/**
*/
EntityPool::EntityPool()
{
    this->generations.Reserve(1024);
    this->freeIds.Reserve(2048);
}

//------------------------------------------------------------------------------
/**
*/
bool
EntityPool::Allocate(Entity& e)
{
    // make sure we don't run out of generations too fast
    // by always deallocating at least n entities before recycling
    if (this->freeIds.Size() < 1024)
    {
        this->generations.Append(0);
        e.index = this->generations.Size() - 1;
        n_assert2(e.index < 0x003FFFFF, "index overflow");
        e.generation = 0;
        return false;
    }
    else
    {
        uint32_t const index = this->freeIds.Dequeue();
        e.index = index;
        e.generation = this->generations[index];
        return true;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
EntityPool::Deallocate(Entity e)
{
    n_assert2(this->IsValid(e), "Tried to deallocate invalid/destroyed entity!");
    this->freeIds.Enqueue(e.index);
#if NEBULA_DEBUG
    // if you get this warning, you might want to consider reserving more bits for the generation.
    n_warn2(this->generations[e.index] == 0x3FF, "Entity generation overflow!");
#endif
    this->generations[e.index]++;

}

//------------------------------------------------------------------------------
/**
*/
bool
EntityPool::IsValid(Entity e) const
{
    return e.index < (uint32_t)this->generations.Size() &&
        e.generation == this->generations[e.index];
}

//------------------------------------------------------------------------------
/**
*/
World::World()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
World::~World()
{
    this->db = nullptr;
}

//------------------------------------------------------------------------------
/**
*/
CategoryId
World::CreateCategory(CategoryCreateInfo const& info)
{
    CategoryHash catHash;
    // note that the hash never contains the owner property id
    for (int i = 0; i < info.properties.Size(); i++)
    {
        catHash.AddToHash(info.properties[i].id);
    }

    if (this->catIndexMap.Contains(catHash))
    {
#if NEBULA_DEBUG
        // verify that the category's signature is the same as what we're actually trying to create.
        // failing an assert here should be EXTREMELY unlikely.
        Category const& cat = this->categoryArray[this->catIndexMap[catHash].id];

        MemDb::Table const& table = this->db->GetTable(cat.instanceTable);
        SizeT numProperties = info.properties.Size();
        if (info.properties[0] != GameServer::Singleton->state.ownerId)
            numProperties++; // + 1 for the owner property

        n_assert(table.properties.Size() == numProperties);
        for (int i = 0; i < table.properties.Size(); i++)
            n_assert(info.properties.FindIndex(table.properties[i]) != InvalidIndex);
#endif

        return this->catIndexMap[catHash];
    }

    Category cat;
    constexpr ushort NUM_PROPS = 256;
    PropertyId properties[NUM_PROPS];

    MemDb::TableCreateInfo tableInfo;
    tableInfo.name = info.name;
    if (info.properties[0] != GameServer::Singleton->state.ownerId)
    {
        // push owner id into the property array
        const SizeT tableSize = 1 + info.properties.Size();
        n_assert(tableSize < NUM_PROPS);
        tableInfo.numProperties = tableSize;
        tableInfo.properties = properties;

        // always add owner as first column
        properties[0] = GameServer::Singleton->state.ownerId;
        for (int i = 1; i < tableSize; i++)
        {
            properties[i] = info.properties[i - 1];
        }
    }
    else
    {
        const SizeT tableSize = info.properties.Size();
        tableInfo.numProperties = tableSize;
        tableInfo.properties = info.properties.begin();
    }

    // Create an instance table
    cat.instanceTable = this->db->CreateTable(tableInfo);

    // "Prefilter" the processors with the new table (insert the table in the cache that accepts it)
    GameServer::Instance()->AddTableToCaches(cat.instanceTable, this->db->GetTableSignature(cat.instanceTable));

    // Find all managed properties
    int numManaged = 0;
    for (int i = 0; i < info.properties.Size(); i++)
    {
        if ((MemDb::TypeRegistry::Flags(info.properties[i]) & PropertyFlags::PROPERTYFLAG_MANAGED) == PropertyFlags::PROPERTYFLAG_MANAGED)
        {
            properties[numManaged] = info.properties[i];
            numManaged++;
            n_assert(numManaged < NUM_PROPS);
        }
    }

    // Managed properties table
    if (numManaged > 0)
    {
        MemDb::TableCreateInfo managedTableInfo;
        managedTableInfo.name = "<MNGD>:" + info.name;
        managedTableInfo.properties = properties;
        managedTableInfo.numProperties = numManaged;
        cat.managedPropertyTable = this->db->CreateTable(managedTableInfo);
        GameServer::Instance()->AddTableToCaches(cat.managedPropertyTable, this->db->GetTableSignature(cat.managedPropertyTable));
    }
    else
        cat.managedPropertyTable = MemDb::TableId::Invalid();

    cat.hash = catHash;

#ifdef NEBULA_DEBUG
    cat.name = info.name;
#endif

    CategoryId cid = this->categoryArray.Size();

    this->catIndexMap.Add(catHash, cid.id);
    this->categoryArray.Append(cat);

    return cid;
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
World::AllocateInstance(Entity entity, CategoryId category)
{
    n_assert(IsValid(entity));
    n_assert(category < this->categoryArray.Size());

    if (entity.index < this->entityMap.Size() && this->entityMap[entity.index].instance != Game::InstanceId::Invalid())
    {
        n_warning("Entity already registered!\n");
        return InvalidIndex;
    }

    Category& cat = this->categoryArray[category.id];
    InstanceId instance = this->db->AllocateRow(cat.instanceTable);

    this->entityMap[entity.index] = { category, instance };

    // Just make sure the first column in always owner!
    n_assert(this->db->GetColumnId(cat.instanceTable, GameServer::Singleton->state.ownerId) == 0);

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)this->db->GetBuffer(cat.instanceTable, 0);
    owners[instance.id] = entity;

    return instance;
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
World::AllocateInstance(Entity entity, BlueprintId blueprint)
{
    n_assert(IsValid(entity));

    if (entity.index < this->entityMap.Size() && this->entityMap[entity.index].instance != Game::InstanceId::Invalid())
    {
        n_warning("Entity already registered!\n");
        return InvalidIndex;
    }

    EntityMapping mapping = BlueprintManager::Instance()->Instantiate(blueprint);
    this->entityMap[entity.index] = mapping;

    Category const& cat = this->GetCategory(mapping.category);
    // Just make sure the first column in always owner!
    n_assert(this->db->GetColumnId(cat.instanceTable, GameServer::Singleton->state.ownerId) == 0);

    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)this->db->GetBuffer(cat.instanceTable, 0);
    owners[mapping.instance.id] = entity;

    return mapping.instance;
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
World::AllocateInstance(Entity entity, TemplateId templateId)
{
    n_assert(IsValid(entity));

    if (entity.index < this->entityMap.Size() && this->entityMap[entity.index].instance != Game::InstanceId::Invalid())
    {
        n_warning("Entity already registered!\n");
        return InvalidIndex;
    }

    EntityMapping mapping = BlueprintManager::Instance()->Instantiate(templateId);
    this->entityMap[entity.index] = mapping;

    Category const& cat = this->GetCategory(mapping.category);
    // Set the owner of this instance
    Game::Entity* owners = (Game::Entity*)this->db->GetBuffer(cat.instanceTable, 0);
    owners[mapping.instance.id] = entity;

    return mapping.instance;
}

//------------------------------------------------------------------------------
/**
*/
void
World::DeallocateInstance(Entity entity)
{
    CategoryId const category = this->entityMap[entity.index].category;
    n_assert(category < this->categoryArray.Size());

    Category& cat = this->categoryArray[category.id];
    InstanceId& instance = this->entityMap[entity.index].instance;

    n_assert(instance != Game::InstanceId::Invalid());

    if (cat.managedPropertyTable == MemDb::TableId::Invalid())
        this->db->DeallocateRow(cat.instanceTable, instance.id);
    else
    {
        // migrate to managed property table so that we can allow the managers
        // to clean up any externally allocated resources.
        this->db->MigrateInstance(cat.instanceTable, instance.id, cat.managedPropertyTable, false);
    }

    instance = InstanceId::Invalid();
}

//------------------------------------------------------------------------------
/**
*/
InstanceId
World::Migrate(Entity entity, CategoryId newCategory)
{
    EntityMapping mapping = GetEntityMapping(entity);
    Category const& oldCat = this->GetCategory(mapping.category);
    Category const& newCat = this->GetCategory(newCategory);
    InstanceId newInstance = this->db->MigrateInstance(oldCat.instanceTable, mapping.instance.id, newCat.instanceTable, false);
    DefragmentCategoryInstances(oldCat);
    this->entityMap[entity.index] = { newCategory, newInstance };
    return newInstance;
}

//------------------------------------------------------------------------------
/**
    @param newInstances     Will be filled with the new instance ids in the destination category.
    @note   This assumes ALL entities in the entity array is of same category!
*/
void
World::Migrate(Util::Array<Entity> const& entities, CategoryId fromCategory, CategoryId newCategory, Util::FixedArray<IndexT>& newInstances)
{
    if (newInstances.Size() != entities.Size())
    {
        newInstances.SetSize(entities.Size());
    }

    Util::Array<IndexT> instances;
    SizeT const num = entities.Size();
    instances.Reserve(num);

    for (auto entity : entities)
    {
        EntityMapping mapping = GetEntityMapping(entity);
#ifdef NEBULA_DEBUG
        n_assert(mapping.category == fromCategory);
#endif // NEBULA_DEBUG
        instances.Append(mapping.instance.id);
    }

    Category const& oldCat = this->GetCategory(fromCategory);
    Category const& newCat = this->GetCategory(newCategory);

    this->db->MigrateInstances(oldCat.instanceTable, instances, newCat.instanceTable, newInstances, false);
    DefragmentCategoryInstances(oldCat);
    for (IndexT i = 0; i < num; i++)
    {
        this->entityMap[entities[i].index] = { newCategory, (uint32_t)newInstances[i] };
    }
}

} // namespace Game
