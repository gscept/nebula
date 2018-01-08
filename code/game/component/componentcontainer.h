#pragma once
//------------------------------------------------------------------------------
/**
    @class Game::ComponentDataData

    Base ComponentData that provides mapping from Game::Ids to an internal array index for ComponentDatas
    Can perform garbage collection that rearranges array entries to avoid gaps
    
    (C) 2017 Individual contributors, see AUTHORS file
*/

#include "util/dictionary.h"
#include "ids/id.h"


//-----------------------------------------------------------------------------
namespace Game
{
template<typename InstanceData>
class ComponentDataContainer
{
    public:
    ///
    ComponentDataContainer();
    ///
    void Allocate(uint32_t idx);

    Util::Array<InstanceData> data;
};

template<typename InstanceData>
void
ComponentDataContainer<InstanceData>::Allocate(uint32_t idx)
{
    InstanceData foo;
    data.Append(foo);
    data.Append(foo);
    &data[0] = nullptr;

}


}