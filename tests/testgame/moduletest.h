#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::ModuleTest

    Tests runtime module loading behavior.

    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

namespace Test
{
class ModuleTest : public TestCase
{
    __DeclareClass(ModuleTest);
public:
    virtual void Run();
};
}
//------------------------------------------------------------------------------
