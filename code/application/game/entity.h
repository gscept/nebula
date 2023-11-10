#pragma once
//------------------------------------------------------------------------------
/**
    @struct Game::Entity

    An entity is essentially just an Id with some utility functions attached.
    What actually makes up the entities are their components.

    The id is split into two parts: the upper 10 bits are used as a generation
    counter, so that we can easily reuse the lower 22 bits as an index.
    
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

//------------------------------------------------------------------------------
/**
*/
struct Entity
{
    uint32_t index : 22;      // 4M concurrent entities
    uint32_t generation : 10; // 1024 generations per index

    Entity() = default;
    constexpr Entity(uint32_t id);
    constexpr Entity(uint32_t generation, uint32_t index);

    static Entity FromId(Ids::Id32 id);

    explicit constexpr operator Ids::Id32() const;
    static constexpr Entity Invalid();
    constexpr uint32_t HashCode() const;
    Entity& operator=(uint32_t rhs);
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
    static constexpr size_t num_fields = 2;
    static constexpr const char* field_names[num_fields] = {
        "index",
        "generation"
    };
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
inline constexpr Entity::Entity(uint32_t id)
{
    this->index = id & 0x003FFFFF;
    this->generation = (id & 0xFFC0000) >> 22;
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Entity::Entity(uint32_t generation, uint32_t index)
{
    this->index = index;
    this->generation = generation;
}

//------------------------------------------------------------------------------
/**
*/
inline Entity
Entity::FromId(Ids::Id32 id)
{
    Entity ret;
    ret.index = id & 0x003FFFFF;
    ret.generation = (id & 0xFFC0000) >> 22;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Game::Entity::operator Ids::Id32() const
{
    return ((generation << 22) & 0xFFC0000) + (index & 0x003FFFFF);
}

//------------------------------------------------------------------------------
/**
*/
inline constexpr Entity
Entity::Invalid()
{
    return {0xFFFFFFFF, 0xFFFFFFFF};
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
Entity::operator=(uint32_t id)
{
    this->index = id & 0x003FFFFF;
    this->generation = (id & 0xFFC0000) >> 22;
    return *this;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
Entity::operator==(const Entity& rhs) const
{
    return Ids::Id32(*this) == Ids::Id32(rhs);
}

//------------------------------------------------------------------------------
/**
*/
inline const bool
Entity::operator!=(const Entity& rhs) const
{
    return Ids::Id32(*this) != Ids::Id32(rhs);
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
