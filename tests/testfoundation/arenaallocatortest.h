#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::ArenaAllocatorTest

    Test Memory::ArenaAllocator functionality

    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class ArenaAllocatorTest : public TestCase
{
    __DeclareClass(ArenaAllocatorTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
