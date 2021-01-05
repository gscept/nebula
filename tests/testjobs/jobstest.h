#pragma once
//------------------------------------------------------------------------------
/**
    Tests jobs
    
    (C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "testbase/testcase.h"
namespace Test
{
class JobsTest : public TestCase
{
    __DeclareClass(JobsTest);
public:
    /// run test
    virtual void Run();
};
} // namespace Test