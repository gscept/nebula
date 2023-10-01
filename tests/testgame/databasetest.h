#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::DatabaseTest

    Tests MemDb::Database functionality.

    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class DatabaseTest : public TestCase
{
    __DeclareClass(DatabaseTest);

public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
