#ifndef TEST_FIXEDTABLETEST_H
#define TEST_FIXEDTABLETEST_H
//------------------------------------------------------------------------------
/**
    @class Test::FixedTableTest
    
    Test FixedTable functionality
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class FixedTableTest : public TestCase
{
    __DeclareClass(FixedTableTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
    