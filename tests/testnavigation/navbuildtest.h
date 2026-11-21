#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::NavBuildTest

    Tests the tiled navmesh builder.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class NavBuildTest : public TestCase
{
    __DeclareClass(NavBuildTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
