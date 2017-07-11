#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::ComponentDataData

    Base ComponentData that provides mapping from Game::Ids to an internal array index for ComponentDatas
    Can perform garbage collection that rearranges array entries to avoid gaps
    
    (C) 2017 Individual contributors, see AUTHORS file
*/

#include "stdneb.h"
#include "util/dictionary.h"
#include "id/id.h"
#include "componentcontainer.h"


//-----------------------------------------------------------------------------
namespace Game
{
class ComponentData
{
    public:
    ///
    ComponentData();
    ///
    ~ComponentData();

    /// register an Id. Will create new mapping and allocate instance data
    void RegisterId(Id id);
    /// deregister an Id. will only remove the id and zero the block
    void DeregisterId(Id id);
    /// perform garbage collection
    void Optimize();
    /// retrieve the instance id of an external id for faster lookup
    /// will be made invalid by Optimize()
    uint32_t GetInstance(Id id) const;    
    protected:
    
    /// mapping of id index to array entry in ComponentData
    Util::Dictionary<Id, uint32_t> idMap;
    Util::Queue<uint32_t> freeIds;
};


//------------------------------------------------------------------------------
/**
*/
ComponentData::ComponentData()
{
    this->idMap.Reserve(1024);
    this->freeIds.Reserve(1024);    
}

//------------------------------------------------------------------------------
/**
*/
ComponentData::~ComponentData()
{
    // empty
}


//------------------------------------------------------------------------------
/**
*/
void
ComponentData::RegisterId(Id id)
{
    if(this->freeIds.IsEmpty())
    {
       /* uint32_t next = this->data.Size();
        InstanceData i;        
        this->data.Append(i);
        this->idMap.Add(id, next);*/        
    }
    else
    {
        uint32_t next = this->freeIds.Dequeue();        
        this->idMap.Add(id, next);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ComponentData::DeregisterId(Id id)
{
    n_assert(this->idMap.Contains(id));
    uint32_t idx = this->idMap[id];    
    this->freeIds.Enqueue(idx);
    this->idMap.Erase(id);
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
ComponentData::GetInstance(Id id) const
{
    n_assert(this->idMap.Contains(id));
    return this->idMap[id];
}
}
//-----------------------------------------------------------------------------