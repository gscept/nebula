#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::StackArrayTest

    Tests Nebula's array with item stack.

    (C) 2023 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class StackArrayTest : public TestCase
{
    __DeclareClass(StackArrayTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
