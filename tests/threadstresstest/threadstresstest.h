#pragma once
//------------------------------------------------------------------------------
/**
    Stress tests jobs and safequeue
    
    (C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "testbase/testcase.h"
namespace Test
{
class ThreadStressTest : public TestCase
{
    __DeclareClass(ThreadStressTest);
public:
    /// run test
    virtual void Run();
};
} // namespace Test