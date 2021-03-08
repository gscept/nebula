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

//------------------------------------------------------------------------------
/**
*/
World::World() :
    numEntities(0)
{
    this->db = MemDb::Database::Create();
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
    @todo When cleaning up the db, erase all tables from the cache.
*/
void
World::CacheTable(MemDb::TableId tid, MemDb::TableSignature signature)
{
    // this is just to compress the code a bit
    const Util::Array<CallbackInfo>* cbArrays[] = {
        &this->onBeginFrameCallbacks,
        &this->onFrameCallbacks,
        &this->onEndFrameCallbacks,
        &this->onLoadCallbacks,
        &this->onSaveCallbacks,
    };

    for (auto arrPtr : cbArrays)
    {
        auto const& arr = *arrPtr;
        for (auto& cbinfo : arr)
        {
            if (MemDb::TableSignature::CheckBits(signature, GetInclusiveTableMask(cbinfo.filter)))
            {
                MemDb::TableSignature const& exclusive = GetExclusiveTableMask(cbinfo.filter);
                if (exclusive.IsValid())
                {
                    if (!MemDb::TableSignature::HasAny(signature, exclusive))
                        cbinfo.cache.Append(tid);
                }
                else
                {
                    cbinfo.cache.Append(tid);
                }
            }

        }
    }
}

} // namespace Game
