//------------------------------------------------------------------------------
//  idgenerationpool.cc
//  (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "idgenerationpool.h"

namespace Ids
{

//------------------------------------------------------------------------------
/**
*/
IdGenerationPool::IdGenerationPool() :
    freeIdsSize(0)
{
    // this->freeIds.Reserve(2048);
    this->generations.Reserve(1024);
}

//------------------------------------------------------------------------------
/**
*/
IdGenerationPool::~IdGenerationPool()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
bool
IdGenerationPool::Allocate(Id32& id)
{
    if (this->freeIdsSize < 1024)
    {
        this->generations.Append(0);
        id = CreateId(this->generations.Size() - 1, 0);
        return false;
    }
    else
    {
        this->freeIdsSize--;        
        id = this->freeIds.Dequeue();
        id = CreateId(id, this->generations[id]);
        return true;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
IdGenerationPool::Deallocate(Id32 id)
{
    n_assert2(this->IsValid(id), "Tried to delete invalid/destroyed id");
    this->freeIdsSize++;        
    this->freeIds.Enqueue(Index(id));
    this->generations[Index(id)]++;
}

//------------------------------------------------------------------------------
/**
*/
bool
IdGenerationPool::IsValid(Id32 id) const
{
    return Index(id) < (uint32_t)this->generations.Size() && Generation(id) == this->generations[Index(id)];
}

} // namespace Ids