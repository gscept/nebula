#ifndef TEST_LISTTEST_H
#define TEST_LISTTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::ListTest
    
    Test functionality of Nebula3's List class.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class ListTest : public TestCase
{
    __DeclareClass(ListTest);
public:
    /// run the test
    virtual void Run();
};

};
//------------------------------------------------------------------------------
#endif    