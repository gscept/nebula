//------------------------------------------------------------------------------
//  idgenerationpool.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "idgenerationpool.h"

namespace Ids
{

//------------------------------------------------------------------------------
/**
*/
IdGenerationPool::IdGenerationPool()
{
    this->freeIds.Reserve(1024);
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
	if (this->freeIds.Size() < 1024)
    {
        this->generations.Append(0);
		id = CreateId(this->generations.Size() - 1, 0);
		return false;
    }
    else
    {
        uint32_t id = this->freeIds.Dequeue();
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
    this->freeIds.Enqueue(Index(id));
    this->generations[Index(id)]++;
}

//------------------------------------------------------------------------------
/**
*/
bool
IdGenerationPool::IsValid(Id32 id)
{
    return Index(id) < (uint32_t)this->generations.Size() && Generation(id) == this->generations[Index(id)];
}


} // namespace Ids