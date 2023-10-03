//------------------------------------------------------------------------------
//  entity.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "entity.h"

namespace Game
{

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

} // namespace Game
