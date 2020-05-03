#pragma once
#ifndef TEST_MEMORYPOOLTEST_H
#define TEST_MEMORYPOOLTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::MemoryPoolTest
  
    Test Memory::MemoryPool functionality.
    
    (C) 2008 Radon Labs GmbH
*/    
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class MemoryPoolTest : public TestCase
{
    __DeclareClass(MemoryPoolTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
#endif        

