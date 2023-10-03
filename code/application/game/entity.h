#pragma once
//------------------------------------------------------------------------------
/**
    @struct Game::Entity

    An entity is essentially just an Id with some utility functions attached.
    What actually makes up the entities are their components.

    The id is split into two parts: the upper 10 bits are used as a generation
    counter, so that we can easily reuse the lower 22 bits as an index.
    
    @see    Game::IsValid
    @see    api.h
    @see    component.h
    @see    memdb/typeregistry.h

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
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

    static Entity FromId(Ids::Id32 id);

    explicit constexpr operator Ids::Id32() const;
    static constexpr Entity Invalid();
    constexpr uint32_t HashCode() const;
    const bool operator==(const Entity& rhs) const;
    const bool operator!=(const Entity& rhs) const;
    const bool operator<(const Entity& rhs) const;
    const bool operator>(const Entity& rhs) const;
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
