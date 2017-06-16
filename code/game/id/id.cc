//------------------------------------------------------------------------------
//  id.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "id.h"


namespace Game
{

//------------------------------------------------------------------------------
/**
*/
IdSystem::IdSystem()
{
    this->freeIds.Reserve(1024);
    this->generations.Reserve(1024);
}

//------------------------------------------------------------------------------
/**
*/
IdSystem::~IdSystem()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Id
IdSystem::Allocate()
{
    if(this->freeIds.Size() < 1024)
    {
        this->generations.Append(0);
        return Id::Create(this->generations.Size()-1,0);        
    }
    else
    {
        uint32_t id = this->freeIds.Dequeue();
        return Id::Create(id, this->generations[id]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
IdSystem::Deallocate(Id id)
{
    n_assert2(this->IsValid(id), "tried to delete Invalid/destroyed id");
    this->freeIds.Enqueue(id.Index());
    this->generations[id.Index()]++;
}

//------------------------------------------------------------------------------
/**
*/
bool
IdSystem::IsValid(Id id)
{
    return id.Index() < this->generations.Size() && id.Generation() == this->generations[id.Index()];
}

}