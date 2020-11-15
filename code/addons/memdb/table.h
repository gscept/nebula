#pragma once
//------------------------------------------------------------------------------
/**
    @file   memdb/table.h

    Contains declarations for tables.

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/arrayallocator.h"
#include "util/fixedarray.h"
#include "util/string.h"
#include "util/stringatom.h"
#include "propertyid.h"
#include "tablesignature.h"

namespace MemDb
{

/// TableId contains an id reference to the database it's attached to, and the id of the table.
ID_32_TYPE(TableId);

/// column id
ID_16_TYPE(ColumnIndex);

/// information for creating a table
struct TableCreateInfo
{
    /// name to be given to the table
    Util::String name;
    /// which properties the table should initially have
    Util::FixedArray<PropertyId> columns;
};

//------------------------------------------------------------------------------
/**
    A table hold properties as columns, and buffers for those columns.
    Property descriptions are retrieved from the MemDb::TypeRegistry

    @see    memdb/typeregistry.h
*/
struct Table
{
    using ColumnBuffer = void*;
    /// table identifier
    TableId tid;
    /// name of the table
    Util::StringAtom name;
    /// number of rows
    uint32_t numRows = 0;
    /// total number of rows allocated
    uint32_t capacity = 128;
    /// initial grow. Gets doubled when table is saturated and expanded
    uint32_t grow = 128;
    // holds freed indices/rows to be reused in the table.
    Util::Array<IndexT> freeIds;
    /// arrays that hold the property identifier, and the buffers
    Util::ArrayAllocator<PropertyId, ColumnBuffer> columns;
    /// allocation heap used for the column buffers
    static constexpr Memory::HeapType HEAP_MEMORY_TYPE = Memory::HeapType::DefaultHeap;
};

} // namespace MemDb
