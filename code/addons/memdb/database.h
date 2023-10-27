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
#include "attributeid.h"
#include "attributeregistry.h"
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
    /// find table by signature
    TableId FindTable(TableSignature const& signature) const;
    /// retrieve the number of tables
    SizeT GetNumTables() const;
    /// run a callback for each table in the db
    void ForEachTable(std::function<void(TableId)> const& callback);

    /// performs Clean on all tables.
    void Reset();

    /// Query the database for a dataset of tables
    Dataset Query(FilterSet const& filterset);
    /// Query the database for a set of tables that fulfill the requirements
    Util::Array<TableId> Query(TableSignature const& inclusive, TableSignature const& exclusive);

    /// copy the database into dst
    void Copy(Ptr<MemDb::Database> const& dst) const;

    // @note    Keep this a fixed size array, because we want to be able to keep persistent references to the tables, and their buffers within
    static constexpr uint32_t MAX_NUM_TABLES = 512;

private:
    /// id pool for table ids
    Ids::IdGenerationPool tableIdPool;

    /// all tables within the database
    Table tables[MAX_NUM_TABLES];

    /// number of tables existing currently
    SizeT numTables = 0;
};

//------------------------------------------------------------------------------
/**
*/
inline SizeT
Database::GetNumTables() const
{
    return this->numTables;
}

} // namespace MemDb
