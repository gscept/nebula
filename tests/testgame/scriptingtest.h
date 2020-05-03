#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::ScriptingTest
    
    Tests Python scripting functionality.
    
    (C) 2019 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class ScriptingTest : public TestCase
{
    __DeclareClass(ScriptingTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
