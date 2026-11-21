#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::NavAssetTest

    Tests baking navmesh work files and loading the baked data.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class NavAssetTest : public TestCase
{
    __DeclareClass(NavAssetTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
