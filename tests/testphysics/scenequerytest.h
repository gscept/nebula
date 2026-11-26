#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::SceneQueryTest

    Tests scene queries against a physics scene.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class SceneQueryTest : public TestCase
{
    __DeclareClass(SceneQueryTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
