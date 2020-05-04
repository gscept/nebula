#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::LoaderTest
    
    Tests Loader namespace functionality.
    
    (C) 2018 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class LoaderTest : public TestCase
{
    __DeclareClass(LoaderTest);
public:
    /// run the test
    virtual void Run();
};

} // namespace Test
//------------------------------------------------------------------------------
