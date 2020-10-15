#pragma once
//------------------------------------------------------------------------------
/**
    MemDb::Database

    In-memory, minimally (memory) fragmented, non-relational database.

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/arrayallocator.h"
#include "table.h"
#include "columnview.h"
#include "util/stringatom.h"
#include "ids/idgenerationpool.h"
#include "columndescription.h"
#include "typeregistry.h"

namespace MemDb
{

struct FilterSet
{
    /// categories must include all attributes in this array
    Util::Array<ColumnDescriptor> inclusive;
    /// categories must NOT contain any attributes in this array
    Util::Array<ColumnDescriptor> exclusive;
};

struct Dataset
{
    /// A view into a category table.
    struct View
    {
        TableId tid;
        SizeT numInstances = 0;
        Util::ArrayStack<void*, 16> buffers;
    };

    /// the filter that has been used to attain this dataset
    FilterSet filter;

    /// views into the tables
    Util::Array<View> tables;
};

class Database : public Core::RefCounted
{
    __DeclareClass(MemDb::Database);
public:
    Database();
    ~Database();

    TableId CreateTable(TableCreateInfo const& info);
    void DeleteTable(TableId table);
    bool IsValid(TableId table) const;
    
    bool HasColumn(TableId table, ColumnDescriptor col);

    /// Returns a type descriptor
    ColumnDescriptor GetColumn(TableId table, ColumnId columnId);

    ColumnId GetColumnId(TableId table, ColumnDescriptor column);

    ColumnId AddColumn(TableId table, ColumnDescriptor column);

    IndexT AllocateRow(TableId table);
    void DeallocateRow(TableId table, IndexT row);

    /// Set all row values to default
    void SetToDefault(TableId table, IndexT row);

    /// Set an attribute value
    //void Set(TableId table, ColumnId columnId, IndexT row, AttributeValue const& value);

    /// get number of rows in a table
    SizeT GetNumRows(TableId table) const;

    /// Get the type descriptors for a table
    Util::Array<ColumnDescriptor> const& GetColumns(TableId table);

    /// gets a custom state data column from table.
    template<typename TYPE>
    const ColumnView<typename TYPE> GetColumnView(TableId tid, ColumnDescriptor descriptor);
    
    /// Get a persistant buffer. Only use this if you know what you're doing!
    void** GetPersistantBuffer(TableId table, ColumnId cid);
    
    /// retrieve a table.
    Table& GetTable(TableId tid);

    /// defragment table
    SizeT Defragment(TableId tid, std::function<void(IndexT, IndexT)> const& moveCallback);

    /// Query the database for a dataset of categories
    Dataset Query(FilterSet const& filterset);

private:
    void EraseSwapIndex(Table& table, IndexT instance);

    void GrowTable(TableId tid);

    void* AllocateBuffer(TableId tid, ColumnDescription* desc);

    Ids::IdGenerationPool tableIdPool;

    // @note    Keep this a fixed size array, because we want to be able to keep persistent references to the tables, and their buffers within
    static constexpr uint32_t MAX_NUM_TABLES = 512;
    Table tables[MAX_NUM_TABLES];
    SizeT numTables = 0;
};

//------------------------------------------------------------------------------
/**
*/
inline void**
Database::GetPersistantBuffer(TableId table, ColumnId cid)
{
    n_assert(this->IsValid(table));
    Table& tbl = this->tables[Ids::Index(table.id)];
    void* ptr = &tbl.columns.Get<1>(cid.id);
    return &tbl.columns.Get<1>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
template<typename TYPE>
inline const ColumnView<TYPE>
Database::GetColumnView(TableId tid, ColumnDescriptor descriptor)
{
    n_assert(this->IsValid(tid));
    Table& table = this->tables[Ids::Index(tid.id)];

    auto const& descriptors = table.columns.GetArray<0>();
    for (int i = 0; i < descriptors.Size(); i++)
    {
        auto const& desc = descriptors[i];

        if (desc == descriptor)
        {
            return ColumnView<TYPE>(i, &table.columns.Get<1>(i), &table.numRows);
        }
    }

    n_error("State does not exist in table!\n");

    return ColumnView<TYPE>(NULL, nullptr, nullptr);
}

} // namespace MemDb
