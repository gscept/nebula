#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::Vec4Test
    
    Tests vec4 functionality.
    
    (C) 2020 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class Vec4Test : public TestCase
{
    __DeclareClass(Vec4Test);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
