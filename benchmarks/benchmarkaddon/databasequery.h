#ifndef BENCHMARKING_DATABASEQUERY_H
#define BENCHMARKING_DATABASEQUERY_H
//------------------------------------------------------------------------------
/**
    @class Benchmarking::DatabaseQuery
    
    Measure database query performance. Depends on the database created
    in the DatabaseInsert benchmarking class.
    
    (C) 2006 Radon Labs GmbH
*/
#include "benchmarkbase/benchmark.h"
#include "addons/db/database.h"

//------------------------------------------------------------------------------
namespace Benchmarking
{
class DatabaseQuery : public Benchmark
{
    __DeclareClass(DatabaseQuery);
public:
    /// run the benchmark
    virtual void Run(Timing::Timer& timer);

private:
    /// run a simple query on an indexed column
    void RunIndexedQuery(Db::Database* db);
    /// run a simple query on a non-indexed column
    void RunNonIndexedQuery(Db::Database* db);
};

}
//------------------------------------------------------------------------------
#endif
