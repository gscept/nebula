#ifndef TEST_IOINTERFACETEST_H
#define TEST_IOINTERFACETEST_H
//------------------------------------------------------------------------------
/**
    @class Test::IOInterfaceTest
    
    Tests asynchronous IO::Interface functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class IOInterfaceTest : public TestCase
{
    __DeclareClass(IOInterfaceTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
        