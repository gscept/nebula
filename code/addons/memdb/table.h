#pragma once
//------------------------------------------------------------------------------
/**
    Game::Table

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/arrayallocator.h"
#include "util/fixedarray.h"
#include "util/string.h"
#include "util/stringatom.h"

namespace MemDb
{

/// TableId contains an id reference to the database it's attached to, and the id of the table.
ID_32_TYPE(TableId);

constexpr uint16_t MAX_VALUE_TABLE_COLUMNS = 128;

/// column id
ID_16_TYPE(ColumnId);
ID_16_TYPE(ColumnDescriptor);

struct TableCreateInfo
{
    Util::String name;
    Util::FixedArray<ColumnDescriptor> columns;
};

//------------------------------------------------------------------------------
/**
    A table describes and holds columns, and buffers for those columns.
    
    Tables can also contain state columns, that are specific to a certain context only.
*/
struct Table
{
    using ColumnBuffer = void*;

    Util::StringAtom name;

    uint32_t numRows = 0;
    uint32_t capacity = 128;
    uint32_t grow = 128;
    // Holds freed indices to be reused in the attribute table.
    Util::Array<IndexT> freeIds;

    Util::ArrayAllocator<ColumnDescriptor, ColumnBuffer> columns;

    static constexpr Memory::HeapType HEAP_MEMORY_TYPE = Memory::HeapType::ObjectArrayHeap;
};

} // namespace MemDb
