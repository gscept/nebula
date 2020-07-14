#ifndef TEST_FIXEDARRAYTEST_H
#define TEST_FIXEDARRAYTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::FixedArrayTest
    
    Test FixedArray functionality
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class FixedArrayTest : public TestCase
{
    __DeclareClass(FixedArrayTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
    