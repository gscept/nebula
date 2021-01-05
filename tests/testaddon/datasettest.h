#ifndef TEST_DATASETTEST_H
#define TEST_DATASETTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::DatasetTest
    
    Test Dataset functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"
#include "db/database.h"
#include "db/table.h"

//------------------------------------------------------------------------------
namespace Test
{
class DatasetTest : public TestCase
{
    __DeclareClass(DatasetTest);
public:
    /// run the test
    virtual void Run();

private:
    /// populate the test database with data
    void PopulateDatabase(Db::Database* db);
    /// populate the persons table with data
    void PopulatePersonsTable(Db::Dataset* t);
    /// populate the products table with data
    void PopulateProductsTable(Db::Dataset* t);
    /// populate the orders table with data
    void PopulateOrdersTable(Db::Dataset* t);
    /// populate the cities table with data
    void PopulateCitiesTable(Db::Dataset* t);
};

} // namespace Test
//------------------------------------------------------------------------------
#endif
    