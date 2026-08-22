#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::ProcessTest

    Tests the System::Process API.
    
    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class ProcessTest : public TestCase
{
    __DeclareClass(ProcessTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
