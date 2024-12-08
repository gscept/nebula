#pragma once
//------------------------------------------------------------------------------
/**
    @struct Game::Entity

    An entity is essentially just an Id with some utility functions attached.
    What actually makes up the entities are their components.

    The id is split into two parts: The upper half is world id, the upper 10 bits of the lower half are used as a generation
    counter, so that we can easily reuse the lowest 22 bits as an index.
    
    @see    Game::World::IsValid
    @see    api.h
    @see    component.h
    @see    world.h

    @copyright
    (C) 2018-2023 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "memdb/tableid.h"

namespace Game
{

// TODO: move this to a separate file
typedef uint32_t WorldId;
ID_32_TYPE(WorldHash);

//------------------------------------------------------------------------------
/**
*/
struct Entity
{
    uint64_t index : 22;      // 4M concurrent entities
    uint64_t generation : 10; // 1024 generations per index
    uint64_t world : 8;       // world id (not to be confused with world hash)
    uint64_t reserved : 24;   // not currently in use

    Entity() = default;
    constexpr Entity(uint64_t id);
    constexpr Entity(uint64_t world, uint64_t generation, uint64_t index);

    static constexpr Entity FromId(Ids::Id64 id);

    explicit constexpr operator Ids::Id64() const;
    static constexpr Entity Invalid();
    constexpr uint32_t HashCode() const;
    Entity& operator=(uint64_t rhs);
    const bool operator==(const Entity& rhs) const;
    const bool operator!=(const Entity& rhs) const;
    const bool operator<(const Entity& rhs) const;
    const bool operator>(const Entity& rhs) const;

    struct Traits;
};

//------------------------------------------------------------------------------
/**
*/
struct Entity::Traits
{
    Traits() = delete;
    using type = Entity;
    static constexpr auto name = "Entity";
    static constexpr auto fully_qualified_name = "Game.Entity";
    static constexpr size_t num_fields = 1;
    static constexpr const char* field_names[num_fields] = {
        "id"
    };
    static constexpr const char* field_typenames[num_fields] = {
        "uint64"
    };
    static constexpr size_t field_byte_offsets[num_fields] = { 0 };

    /// This is the column that the entity "owner" will reside in, in every table.
    /// NOTE: This can never be changed, due to assumptions that have been made.
    static constexpr uint32_t fixed_column_index = 0;
};

//------------------------------------------------------------------------------
/**
    Maps an entity to a table and instance id
*/
struct EntityMapping
{
    MemDb::TableId table;
    MemDb::RowId instance;
};

//------------------------------------------------------------------------------
/**
*/
inline constexpr Entity::Entity(uint64_t id)
{
    this->index = id & 0x00000000003FFFFF;
    this->generation = (id & 0x0000000FFC00000) >> 22;
    this->world = (id & 0x000000FF00000000) >> 32;
    this->reserved = 0x0;
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Entity::Entity(uint64_t world, uint64_t generation, uint64_t index)
{
    this->index = index;
    this->generation = generation;
    this->world = world;
    this->reserved = 0x0;
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Entity
Entity::FromId(Ids::Id64 id)
{
    Entity ret = Entity((uint64_t)id);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Game::Entity::operator Ids::Id64() const
{
    return (((uint64_t)world << 32) & 0x000000FF00000000)
         + (((uint64_t)generation << 22) & 0x0000000FFC00000)
         + ((uint64_t)index & 0x00000000003FFFFF);
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Entity
Entity::Invalid()
{
    return 0xFFFFFFFFFFFFFFFF;
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr uint32_t
Entity::HashCode() const
{
    return index;
}

//------------------------------------------------------------------------------
/**
*/
inline Entity&
Entity::operator=(uint64_t id)
{
    this->index = id & 0x00000000003FFFFF;
    this->generation = (id & 0x0000000FFC00000) >> 22;
    this->world = (id & 0x000000FF00000000) >> 32;
    this->reserved = (id & 0xFFFFFF0000000000) >> 40;
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
Entity::operator==(const Entity& rhs) const
{
    return Ids::Id64(*this) == Ids::Id64(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
Entity::operator!=(const Entity& rhs) const
{
    return Ids::Id64(*this) != Ids::Id64(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
Entity::operator<(const Entity& rhs) const
{
    return index < rhs.index;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
Entity::operator>(const Entity& rhs) const
{
    return index > rhs.index;
}

} // namespace Game
