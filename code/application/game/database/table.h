#pragma once
//------------------------------------------------------------------------------
/**
    Game::Table

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "game/database/attribute.h"
#include "statedescription.h"
#include "util/arrayallocator.h"

namespace Game
{

namespace Db
{

/// TableId contains an id reference to the database it's attached to, and the id of the table.
ID_32_TYPE(TableId);

constexpr uint16_t MAX_VALUE_TABLE_COLUMNS = 128;

/// column id
ID_16_TYPE(ColumnId);

typedef Game::AttributeId Column;

//------------------------------------------------------------------------------
/**
    A table describes and holds columns, and buffers for those columns.
    
    Tables can also contain state columns, that are specific to a certain context only.
*/
struct Table
{
    using ColumnBuffer = void*;

    Util::StringAtom name;

    Util::ArrayAllocator<Column, ColumnBuffer> columns;
    uint32_t numRows = 0;
    uint32_t capacity = 128;
    uint32_t grow = 128;
    // Holds freed indices to be reused in the attribute table.
    Util::Array<IndexT> freeIds;

    Util::ArrayAllocator<StateDescription, ColumnBuffer> states;

    static constexpr Memory::HeapType HEAP_MEMORY_TYPE = Memory::HeapType::ObjectArrayHeap;
};

} // namespace Db

} // namespace Game
