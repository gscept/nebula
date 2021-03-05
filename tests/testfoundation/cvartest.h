#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::CVarTest
    
    Test blob functionality
    
    (C) 2021 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class CVarTest : public TestCase
{
    __DeclareClass(CVarTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
    