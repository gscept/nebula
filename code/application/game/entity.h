#pragma once
//------------------------------------------------------------------------------
/**
    @struct Game::Entity

    An entity is essentially just an Id with some utility functions attached.
    What actually makes up the entities are their properties.

    The id is split into two parts: the upper 10 bits are used as a generation
    counter, so that we can easily reuse the lower 22 bits as an index.
    
    @see    Game::IsValid
    @see    api.h
    @see    propertyid.h
    @see    memdb/typeregistry.h

    @copyright
    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"

namespace Game
{
    struct Entity
    {
        uint32_t index     : 22; // 4M concurrent entities
        uint32_t generation : 10; // 1024 generations per index
    
        static Entity FromId(Ids::Id32 id)
        {
            Entity ret;
            ret.index = id & 0x003FFFFF;
            ret.generation = (id & 0xFFC0000) >> 22;
            return ret;
        }
        explicit constexpr operator Ids::Id32() const
        {
            return ((generation << 22) & 0xFFC0000) + (index & 0x003FFFFF);
        }
        static constexpr Entity Invalid()
        {
            return { 0xFFFFFFFF, 0xFFFFFFFF };
        }
        constexpr uint32_t HashCode() const
        {
            return index;
        }
        const bool operator==(const Entity& rhs) const { return Ids::Id32(*this) == Ids::Id32(rhs); }
        const bool operator!=(const Entity& rhs) const { return Ids::Id32(*this) != Ids::Id32(rhs); }
        const bool operator<(const Entity& rhs) const { return index < rhs.index; }
        const bool operator>(const Entity& rhs) const { return index > rhs.index; }
    };

} // namespace Game



