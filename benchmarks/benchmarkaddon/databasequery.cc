//------------------------------------------------------------------------------
//  databasequery.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "benchmarkaddon/databasequery.h"
#include "benchmarkaddon/dbattrs.h"
#include "io/ioserver.h"
#include "addons/db/sqlite3/sqlite3factory.h"
#include "addons/db/dataset.h"

namespace Benchmarking
{
__ImplementClass(Benchmarking::DatabaseQuery, 'DBQR', Benchmarking::Benchmark);

using namespace IO;
using namespace Db;
using namespace Util;
using namespace Timing;
using namespace Attr;

//------------------------------------------------------------------------------
/**
*/
void
DatabaseQuery::Run(Timer& timer)
{
    Ptr<Db::DbFactory> dbFactory = Db::Sqlite3Factory::Create();
    
    timer.Start();

    // open the database created by the insert benchmark
    const URI uri("temp:dbinsertbenchmark.db3");
    Ptr<Database> db = dbFactory->CreateDatabase();
    db->SetURI(uri);
    db->SetAccessMode(Database::ReadOnly);
    bool dbOpened = db->Open();
    n_assert(dbOpened);

    /// run some queries with different complexity
    this->RunIndexedQuery(db);
    this->RunNonIndexedQuery(db);

    // close the database
    db->Close();
    timer.Stop();
}

//------------------------------------------------------------------------------
/**
*/
void
DatabaseQuery::RunIndexedQuery(Database* db)
{
    n_assert(0 != db);
    
    // create and configure dataset    
    Ptr<Table> table = db->GetTableByName("MainTable");
    Ptr<Dataset> dataset = table->CreateDataset();
    dataset->AddAllTableColumns();
    Ptr<FilterSet> filter = dataset->Filter();
    filter->AddEqualCheck(Attribute(Attr::Level, "Berlin"));
    
    Timer timer;
    timer.Start();
    dataset->PerformQuery();
    timer.Stop();

    SizeT numResultRows = dataset->Values()->GetNumRows();

    n_printf("**** DatabaseQuery::RunIndexedQuery(): %d result rows in %d ticks, %f seconds\n", \
        numResultRows, 
        timer.GetTicks(), 
        timer.GetTime());
}

//------------------------------------------------------------------------------
/**
*/
void
DatabaseQuery::RunNonIndexedQuery(Database* db)
{
    n_assert(0 != db);
    
    // create and configure dataset
    Ptr<Table> table = db->GetTableByName("MainTable");
    Ptr<Dataset> dataset = table->CreateDataset();
    dataset->AddAllTableColumns();
    Ptr<FilterSet> filter = dataset->Filter();
    filter->AddEqualCheck(Attribute(Attr::Graphics, "Berlin"));
    
    Timer timer;
    timer.Start();
    dataset->PerformQuery();
    timer.Stop();

    SizeT numResultRows = dataset->Values()->GetNumRows();
    n_printf("**** DatabaseQuery::RunNonIndexedQuery(): %d result rows in %d ticks, %f seconds\n", \
        numResultRows, 
        timer.GetTicks(), 
        timer.GetTime());
}

} // namespace Benchmarking