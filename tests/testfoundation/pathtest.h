#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::PathTest

    Tests Nebula's IO::Path class, the URN format resources are addressed with.

    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class PathTest : public TestCase
{
    __DeclareClass(PathTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
