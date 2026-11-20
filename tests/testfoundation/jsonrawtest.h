#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::JsonRawTest

    Test the raw json node, which is what lets a feature contribute and read back a subtree the
    json reader and writer have no type knowledge of.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class JsonRawTest : public TestCase
{
    __DeclareClass(JsonRawTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
