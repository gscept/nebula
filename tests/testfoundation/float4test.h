#pragma once
#ifndef TEST_FLOAT4TEST_H
#define TEST_FLOAT4TEST_H
//------------------------------------------------------------------------------
/**
    @class Test::Float4Test
    
    Tests float4 functionality.
    
    (C) 2007 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class Float4Test : public TestCase
{
    __DeclareClass(Float4Test);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
