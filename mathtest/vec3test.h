#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::Vec3Test
    
    Tests vec3 functionality.
    
    (C) 2020 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class Vec3Test : public TestCase
{
    __DeclareClass(Vec3Test);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
