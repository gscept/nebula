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
#ifdef NEBULA_DEBUG
        Util::String tableName;
#endif
        TableId tid;
        SizeT numInstances = 0;
        Util::ArrayStack<void*, 16> buffers;
    };

    /// the filter that has been used to attain this dataset
    FilterSet filter;

    /// views into the tables
    Util::Array<View> tables;

    /// Validate tables and num instances. This ensures valid data, but does not re-query for new tables.
    void Validate();
private:
    friend class Database;
    Ptr<Database> db;
};

class Database : public Core::RefCounted
{
    __DeclareClass(MemDb::Database);
public:
	/// constructor
    Database();
	/// destructor
    ~Database();
	
	/// create new table
    TableId CreateTable(TableCreateInfo const& info);
	/// delete table
    void DeleteTable(TableId table);
	/// check if table is valid
    bool IsValid(TableId table) const;
	/// retrieve a table.
	Table& GetTable(TableId tid);
	
	/// check if table has a certain column
    bool HasColumn(TableId table, ColumnDescriptor col);
    /// returns a descriptor for a given column id
    ColumnDescriptor GetColumn(TableId table, ColumnId columnId);
	/// returns a column id or invalid if column is missing from table
    ColumnId GetColumnId(TableId table, ColumnDescriptor column);
	/// add a column to a table
    ColumnId AddColumn(TableId table, ColumnDescriptor column);
	/// get the all descriptors for a table
	Util::Array<ColumnDescriptor> const& GetColumns(TableId table);

	/// allocate a row within a table
    IndexT AllocateRow(TableId table);
	/// deallocate a row from a table. This only frees the row for recycling. See ::Defragment
    void DeallocateRow(TableId table, IndexT row);
	/// get number of rows in a table
	SizeT GetNumRows(TableId table) const;
	/// set all row values to default
    void SetToDefault(TableId table, IndexT row);
	
	/// move instance from one table to another.
    IndexT MigrateInstance(TableId srcTid, IndexT srcRow, TableId dstTid);
    /// duplicate instance from one row into destination table.
    IndexT DuplicateInstance(TableId srcTid, IndexT srcRow, TableId dstTid);

	/// defragment table
	SizeT Defragment(TableId tid, std::function<void(IndexT, IndexT)> const& moveCallback);

	/// Query the database for a dataset of categories
	Dataset Query(FilterSet const& filterset);
	/// gets a column view from table.
    template<typename TYPE>
    const ColumnView<TYPE> GetColumnView(TableId tid, ColumnDescriptor descriptor);
    /// get a persistant buffer. Only use this if you know what you're doing!
    void** GetPersistantBuffer(TableId table, ColumnId cid);
	
private:
	/// recycle free row or allocate new row
    IndexT AllocateRowIndex(TableId table);
	/// erase row by swapping with last row and reducing number of rows in table
    void EraseSwapIndex(Table& table, IndexT instance);
	/// grow each column within table
    void GrowTable(TableId tid);
	/// allocate a buffer for a column. Sets all values to default
    void* AllocateBuffer(TableId tid, ColumnDescription* desc);

	/// id pool for table ids
    Ids::IdGenerationPool tableIdPool;

    // @note    Keep this a fixed size array, because we want to be able to keep persistent references to the tables, and their buffers within
    static constexpr uint32_t MAX_NUM_TABLES = 512;
	/// all tables within the database
    Table tables[MAX_NUM_TABLES];
	/// number of tables existing currently
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
