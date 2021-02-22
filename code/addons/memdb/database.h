#pragma once
//------------------------------------------------------------------------------
/**
    MemDb::Database

    In-memory, minimally (memory) fragmented, non-relational database.

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "util/arrayallocator.h"
#include "table.h"
#include "util/stringatom.h"
#include "ids/idgenerationpool.h"
#include "propertyid.h"
#include "typeregistry.h"
#include "dataset.h"
#include "filterset.h"
#include "util/blob.h"

namespace MemDb
{
class Database;

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
    /// retrieve a table signature
    TableSignature const& GetTableSignature(TableId tid) const;
    /// find table by signature
    TableId FindTable(TableSignature const& signature) const;
    /// retrieve the number of tables
    SizeT GetNumTables() const;
    /// run a callback for each table in the db
    void ForEachTable(std::function<void(TableId)> const& callback);

    /// check if table has a certain column
    bool HasProperty(TableId table, PropertyId col);
    /// returns a descriptor for a given column id
    PropertyId GetPropertyId(TableId table, ColumnIndex columnId);
    /// returns a column id or invalid if column is missing from table
    ColumnIndex GetColumnId(TableId table, PropertyId column);
    /// add a column to a table
    ColumnIndex AddColumn(TableId table, PropertyId column, bool updateSignature = true);
    /// get the all descriptors for a table
    Util::Array<PropertyId> const& GetColumns(TableId table);

    /// allocate a row within a table
    IndexT AllocateRow(TableId table);
    /// deallocate a row from a table. This only frees the row for recycling. See ::Defragment
    void DeallocateRow(TableId table, IndexT row);
    /// get number of rows in a table
    SizeT GetNumRows(TableId table) const;
    /// set all row values to default
    void SetToDefault(TableId table, IndexT row);

    /// move instance from one table to another.
    IndexT MigrateInstance(TableId srcTid, IndexT srcRow, TableId dstTid, bool defragment = true, std::function<void(IndexT, IndexT)> const& moveCallback = nullptr);
    /// move instance from one table to a table in another database.
    IndexT MigrateInstance(TableId srcTid, IndexT srcRow, Ptr<Database> const& dstDb, TableId dstTid, bool defragment = true, std::function<void(IndexT, IndexT)> const& moveCallback = nullptr);
    /// duplicate instance from one row into destination table.
    IndexT DuplicateInstance(TableId srcTid, IndexT srcRow, TableId dstTid);
    /// duplicate instance from one row into destination table in a different database.
    IndexT DuplicateInstance(TableId srcTid, IndexT srcRow, Ptr<Database> const& dstDb, TableId dstTid);

    /// move n instances from one table to another.
    void MigrateInstances(TableId srcTid, Util::Array<IndexT> const& srcRows, TableId dstTid, Util::FixedArray<IndexT>& dstRows, bool defragment = true, std::function<void(IndexT, IndexT)> const& moveCallback = nullptr);
    /// duplicate instance from one row into destination table.
    void DuplicateInstances(TableId srcTid, Util::Array<IndexT> const& srcRows, TableId dstTid, Util::FixedArray<IndexT>& dstRows);

    /// defragment table
    SizeT Defragment(TableId tid, std::function<void(IndexT, IndexT)> const& moveCallback);
    /// clean table. Does not deallocate anything; just sets the size of the table to zero.
    void Clean(TableId tid);
    /// performs Clean on all tables.
    void Reset();

    /// Query the database for a dataset of tables
    Dataset Query(FilterSet const& filterset);
    /// Query the database for a set of tables that fulfill the requirements
    Util::Array<TableId> Query(TableSignature const& inclusive, TableSignature const& exclusive);
    /// get a buffer. Might be invalidated if rows are allocated or deallocated
    void* GetValuePointer(TableId table, ColumnIndex cid, IndexT row);
    /// get a buffer. Might be invalidated if rows are allocated or deallocated
    void* GetBuffer(TableId table, ColumnIndex cid);

    /// copy the database into dst
    void Copy(Ptr<MemDb::Database> const& dst) const;

    /// serialize an instance into a blob.
    Util::Blob SerializeInstance(TableId table, IndexT row);
    /// deserialize a blob into an instance 
    void DeserializeInstance(Util::Blob const& data, TableId table, IndexT row);

    // @note    Keep this a fixed size array, because we want to be able to keep persistent references to the tables, and their buffers within
    static constexpr uint32_t MAX_NUM_TABLES = 512;

private:
    /// recycle free row or allocate new row
    IndexT AllocateRowIndex(TableId table);
    /// erase row by swapping with last row and reducing number of rows in table
    void EraseSwapIndex(Table& table, IndexT instance);
    /// grow each column within table
    void GrowTable(TableId tid);
    /// allocate a buffer for a column. Sets all values to default
    void* AllocateBuffer(TableId tid, PropertyDescription* desc);

    /// id pool for table ids
    Ids::IdGenerationPool tableIdPool;

    /// all tables within the database
    Table tables[MAX_NUM_TABLES];
    /// all table signatures
    TableSignature tableSignatures[MAX_NUM_TABLES];

    /// number of tables existing currently
    SizeT numTables = 0;
};

//------------------------------------------------------------------------------
/**
*/
inline void*
Database::GetValuePointer(TableId table, ColumnIndex cid, IndexT row)
{
    n_assert(this->IsValid(table));
    Table& tbl = this->tables[Ids::Index(table.id)];
    PropertyId descriptor = tbl.columns.Get<0>(cid.id);
    PropertyDescription* desc = MemDb::TypeRegistry::GetDescription(descriptor);
    if (desc->typeSize > 0)
        return ((byte*)tbl.columns.Get<1>(cid.id)) + (desc->typeSize * row);

    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
inline void*
Database::GetBuffer(TableId table, ColumnIndex cid)
{
    n_assert(this->IsValid(table));
    Table& tbl = this->tables[Ids::Index(table.id)];
    return tbl.columns.Get<1>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Database::GetNumTables() const
{
    return this->numTables;
}

} // namespace MemDb
