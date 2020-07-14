#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::BBoxTest
    
    Tests bbox functionality.
    
    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class BBoxTest : public TestCase
{
    __DeclareClass(BBoxTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
