#ifndef TEST_ARRAYTEST_H
#define TEST_ARRAYTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::ArrayTest
    
    Tests Nebula3's dynamic array class.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class ArrayTest : public TestCase
{
    __DeclareClass(ArrayTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
    