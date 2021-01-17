#pragma once
//------------------------------------------------------------------------------
/**
    @struct Game::Entity

    An entity is essentially just an Id with some utility functions attached.
    What actually makes up the entities are their properties.

    The id is split into two parts: the 8 upper bits are used as a generation
    counter, so that we can easily reuse the lower 24 bits as an index.
    
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
    /// category id
    ID_32_TYPE(CategoryId);

    /// instance id point into a category table. Entities are mapped to instanceids
    ID_32_TYPE(InstanceId);

    /// 8+24 bits entity id (generation+index)
    ID_32_TYPE(Entity);
} // namespace Game



