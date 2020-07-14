#ifndef TEST_MEMORYSTREAMTEST_H
#define TEST_MEMORYSTREAMTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::MemoryStreamTest
    
    Test IO::MemoryStream functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class MemoryStreamTest : public TestCase
{
    __DeclareClass(MemoryStreamTest);
public:
    /// run the test
    virtual void Run();
};

};
//------------------------------------------------------------------------------
#endif