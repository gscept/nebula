#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::CharacterBuildTest

    Tests building a character controller from plain data, and the records the actor context holds
    for the actor and shape the controller owns.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class CharacterBuildTest : public TestCase
{
    __DeclareClass(CharacterBuildTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
