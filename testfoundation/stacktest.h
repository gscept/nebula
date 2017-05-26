#ifndef TEST_STACKTEST_H
#define TEST_STACKTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::StackTest
    
    Test functionality of Nebula3's Stack class.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class StackTest : public TestCase
{
    __DeclareClass(StackTest);
public:
    /// run the test
    virtual void Run();
};

};
//------------------------------------------------------------------------------
#endif    