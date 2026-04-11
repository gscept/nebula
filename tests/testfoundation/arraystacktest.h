#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::ArrayStackTest
    
    Tests the stack array class.
    
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class ArrayStackTest : public TestCase
{
    __DeclareClass(ArrayStackTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
