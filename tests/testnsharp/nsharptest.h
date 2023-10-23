#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::NSharpTest
    
    Tests C# scripting functionality.
    
    (C) 2023 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"
#include <functional>

//------------------------------------------------------------------------------
namespace Test
{
class NSharpTest : public TestCase
{
    __DeclareClass(NSharpTest);
public:
    /// run the test
    virtual void Run();

    std::function<void()> StepFrame;
};

} // namespace Test
//------------------------------------------------------------------------------
