#pragma once
#ifndef TEST_RINGBUFFERTEST_H
#define TEST_RINGBUFFERTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::RingBufferTest
    
    Test RingBuffer functionality.
    
    (C) 2008 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class RingBufferTest : public TestCase
{
    __DeclareClass(RingBufferTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
#endif
    