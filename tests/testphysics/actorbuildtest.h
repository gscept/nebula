#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::ActorBuildTest

    Tests building physics actors and shapes from plain descriptions.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class ActorBuildTest : public TestCase
{
    __DeclareClass(ActorBuildTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
