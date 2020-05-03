#ifndef TEST_STREAMSERVERTEST_H
#define TEST_STREAMSERVERTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::StreamServerTest
  
    Test StreamServer functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class StreamServerTest : public TestCase
{
    __DeclareClass(StreamServerTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
