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

} // namespace Game
