#ifndef TEST_SIMPLETREETEST_H
#define TEST_SIMPLETREETEST_H
//------------------------------------------------------------------------------
/**
    @class Test::SimpleTreeTest
    
    Test SimpleTree functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class SimpleTreeTest : public TestCase
{
    __DeclareClass(SimpleTreeTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
