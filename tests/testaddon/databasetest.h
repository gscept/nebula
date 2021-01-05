#ifndef TEST_DATABASETEST_H
#define TEST_DATABASETEST_H
//------------------------------------------------------------------------------
/**
    @class Test::DatabaseTest
  
    Test Db::Database functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class DatabaseTest : public TestCase
{
    __DeclareClass(DatabaseTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
