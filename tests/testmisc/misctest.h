#pragma once
//------------------------------------------------------------------------------
/**
    Tests jobs
    
    (C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "testbase/testcase.h"
namespace Test
{
class MiscTest : public TestCase
{
    __DeclareClass(MiscTest);
public:
    /// run test
    virtual void Run();
};
} // namespace Test