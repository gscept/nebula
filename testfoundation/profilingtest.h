#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::ProfilingTest
    
    Tests Nebula's Profiling system.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class ProfilingTest : public TestCase
{
    __DeclareClass(ProfilingTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
