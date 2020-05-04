#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::IdTest
    
    Tests id functionality.
    
    (C) 2017 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class IdTest : public TestCase
{
    __DeclareClass(IdTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
