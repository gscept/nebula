#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::CookingTest

    Tests the physics cooking layer.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class CookingTest : public TestCase
{
    __DeclareClass(CookingTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
