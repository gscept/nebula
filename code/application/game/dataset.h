#pragma once
//------------------------------------------------------------------------------
/**
    @struct Game::Dataset

    Contains the results of a query made via a Game::World.

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

    //------------------------------------------------------------------------------
    /**
        @struct Game::Dataset::View

        This represents a "view" into an entity table. This is likely only part
        of the table and will rarely contain all entities of a table. Entities are
        split into multiple table views due to memory, performance and threading reasons.
    */
    struct View
    {
        /// table identifier
        MemDb::TableId tableId = MemDb::TableId::Invalid();
        /// partition identifier
        uint16_t partitionId = 0xFFFF;
        /// number of instances in view
        uint16_t numInstances = 0;
        /// component buffers. @note Can be NULL if a queried component has no fields
        void* buffers[MAX_COMPONENT_BUFFERS];
        /// which instances are valid in this buffer
        decltype(MemDb::Table::Partition::validRows) validInstances;
    };

    /// number of views in views array
    uint32_t numViews = 0;
    /// views into the tables
    View* views = nullptr;
};

} // namespace Game
