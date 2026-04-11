//------------------------------------------------------------------------------
//  @file entitypool.cc
//  @copyright (C) 2021 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

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
    memset(&e, 0, sizeof(Entity));
    // make sure we don't run out of generations too fast
    // by always deallocating at least n entities before recycling
    if (this->freeIds.Size() < 1024)
    {
        this->generations.Append(0);
        e.index = this->generations.Size() - 1;
        n_assert2(e.index < 0x003FFFFF, "index overflow");
        e.generation = 0;
        return true;
    }
    else
    {
        uint32_t const index = this->freeIds.Dequeue();
        e.index = index;
        e.generation = this->generations[index];
        return false;
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
    n_warn2(this->generations[e.index] <= 0x3FF, "Entity generation overflow!");
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

} // namespace Game
