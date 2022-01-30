#pragma once
//------------------------------------------------------------------------------
/**
    @file dataset.h

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "memdb/table.h"

namespace Game
{

//------------------------------------------------------------------------------
/**
    A dataset that contains views into category tables. These are created by
    querying the world database.
*/
struct Dataset
{
    static const uint32_t MAX_COMPONENT_BUFFERS = 64;

    /// a view into a category table
    struct EntityTableView
    {
        /// table identifier
        MemDb::TableId tableId = MemDb::TableId::Invalid();
        /// number of instances in view
        uint32_t numInstances = 0;
        /// component buffers. @note Can be NULL if a queried component is a flag
        void* buffers[MAX_COMPONENT_BUFFERS];
    };

    /// number of views in views array
    uint32_t numViews = 0;
    /// views into the tables
    EntityTableView* views = nullptr;
};

} // namespace Game
