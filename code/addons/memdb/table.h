#pragma once
//------------------------------------------------------------------------------
/**
    @file   memdb/table.h

    Contains declarations for tables.

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "util/arrayallocator.h"
#include "util/fixedarray.h"
#include "util/string.h"
#include "util/stringatom.h"
#include "util/hashtable.h"
#include "componentid.h"
#include "tablesignature.h"
#include "util/bitfield.h"

namespace MemDb
{

/// Table identifier
ID_32_TYPE(TableId);

/// row identifier
typedef IndexT Row;

constexpr Row InvalidRow = -1;

/// column id
ID_16_TYPE(ColumnIndex);

/// information for creating a table
struct TableCreateInfo
{
    /// name to be given to the table
    Util::String name;
    /// array of components the table should initially have
    ComponentId const* components;
    /// number of columns
    SizeT numComponents;
};

//------------------------------------------------------------------------------
/**
    A table hold components as columns, and buffers for those columns.
    Property descriptions are retrieved from the MemDb::TypeRegistry

    @see    memdb/typeregistry.h
    @todo   This should be changed to SOAs
*/
struct Table
{
    using ColumnBuffer = void*;
    /// table identifier
    TableId tid = TableId::Invalid();
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
    /// all components that this table has
    Util::Array<ComponentId> components;
    /// can be used to keep track if the columns has been reallocated
    uint32_t version = 0;

    // TODO: partition the tables into chunks
    //struct Partition
    //{
    //	static constexpr uint	 capacity = 4096;  // total capacity (in elements) that the partition can contain
    //	uint64_t				 version = 0;	   // bump the version if you change anything about the partition
    //	void* buffer = nullptr;					   // contains the data
    //	Util::BitField<capacity> modified;		   // check a bit if the value in the buffer has been modified, and you need to track it
    //	uint32_t			     size = 0;		   // current number of elements in the buffer;
    //};

    /// holds all the column buffers. This excludes non-typed components
    Util::ArrayAllocator<ComponentId, ColumnBuffer> columns;
    /// maps componentid -> index in columns array
    Util::HashTable<ComponentId, IndexT, 32, 1> columnRegistry;
    /// allocation heap used for the column buffers
    static constexpr Memory::HeapType HEAP_MEMORY_TYPE = Memory::HeapType::DefaultHeap;
};

using AttributeId = ComponentId;

//------------------------------------------------------------------------------
/**
*/
class _Table
{
    using ColumnBuffer = void*;

private:
    _Table() = default;

public:
    struct RowId
    {
        uint16_t partition;
        uint16_t index;
    };

    class Partition
    {
    public:
        // total capacity (in elements) that the partition can contain
        static constexpr uint capacity = 256;
        /// number of rows
        uint32_t numRows = 0;
        // holds freed indices/rows to be reused in the table.
        Util::Array<IndexT> freeIds;
        /// holds all the column buffers. This excludes non-typed components
        Util::ArrayAllocator<ComponentId, ColumnBuffer> columns;
        // bump the version if you change anything about the partition
        uint64_t version = 0;
        // check a bit if the row has been modified, and you need to track it
        Util::BitField<capacity> modifiedRows;
        // has this partition been modified lately?
        bool modified;
    };

    /// Check if a column exists in the table
    bool HasAttribute(AttributeId attribute) const;

    /// Returns the attribute located in given column
    AttributeId GetAttributeId(ColumnIndex columnIndex) const;
    /// Returns the index of the attribute or invalid if attribute is missing from table
    ColumnIndex GetAttributeIndex(AttributeId attribute) const;
    /// Get the all descriptors for a table
    Util::Array<AttributeId> const& GetAttributes() const;
    /// Get the table signature
    TableSignature const& GetSignature() const;

    /// Add an attribute to the table
    ColumnIndex AddAttribute(AttributeId attribute, bool updateSignature = true);
    /// Add/Get a free row from the table
    RowId AddRow(TableId table);
    /// Deallocate a row from a table. This only frees the row for recycling. See ::Defragment
    void RemoveRow(TableId table, RowId row);
    /// Get total number of rows in a table
    SizeT GetNumRows(TableId table) const;
    /// Set all row values to default
    void SetToDefault(TableId table, RowId row);

    /// Defragment table
    SizeT Defragment(std::function<void(IndexT, IndexT)> const& moveCallback);
    /// Clean table. Does not deallocate anything; just sets the size of the table to zero.
    void Clean();

    /// get a buffer. Might be invalidated if rows are allocated or deallocated
    void* GetValuePointer(ColumnIndex cid, RowId row);
    /// get a buffer. Might be invalidated if rows are allocated or deallocated
    void* GetBuffer(ColumnIndex cid);

    /// Serialize a row into a blob.
    Util::Blob SerializeInstance(RowId row);
    /// deserialize a blob into a row
    void DeserializeInstance(Util::Blob const& data, RowId row);

private:
    TableSignature signature;

    /// table identifier
    TableId tid = TableId::Invalid();
    /// name of the table
    Util::StringAtom name;

    uint32_t totalNumRows = 0;

    Util::Array<Partition*> partitions;
    /// all components that this table has
    Util::Array<ComponentId> attributes;
    /// maps componentid -> index in columns array
    Util::HashTable<ComponentId, IndexT, 32, 1> columnRegistry;
    /// allocation heap used for the column buffers
    static constexpr Memory::HeapType HEAP_MEMORY_TYPE = Memory::HeapType::DefaultHeap;
};

} // namespace MemDb
