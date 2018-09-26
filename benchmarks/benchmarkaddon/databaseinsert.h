#ifndef BENCHMARKING_DATABASEINSERT_H
#define BENCHMARKING_DATABASEINSERT_H
//------------------------------------------------------------------------------
/**
    @class Benchmarking::DatabaseInsert
    
    Measure database bulk insert performance.
    
    (C) 2006 Radon Labs GmbH
*/
#include "benchmarkbase/benchmark.h"
#include "addons/db/database.h"
#include "util/fixedarray.h"

//------------------------------------------------------------------------------
namespace Benchmarking
{
class DatabaseInsert : public Benchmark
{
    __DeclareClass(DatabaseInsert);
public:
    /// run the benchmark
    virtual void Run(Timing::Timer& timer);

private:
    /// open the database
    Ptr<Db::Database> OpenDatabase(const IO::URI& uri);
    /// close the database
    void CloseDatabase(Db::Database* db);
    /// create a test table in the database
    Ptr<Db::Table> CreateTable(Db::Database* db);
    /// populate the database
    void PopulateDatabase(Db::Database* db, Db::Table* table);
    /// return a pseudo random integer
    int GetRandomInt() const;
    /// return a pseudo random float
    float GetRandomFloat() const;
    /// return a pseudo random string
    const Util::String& GetRandomString() const;
    /// populate the string table for pseudo random string
    void PopulateStringTable();
    /// write a random value to a column/row index
    void WriteRandomValue(Db::ValueTable* table, IndexT colIndex, IndexT rowIndex);

    Util::FixedArray<Util::String> stringTable;
};

} // namespace Benchmark
//------------------------------------------------------------------------------
#endif
