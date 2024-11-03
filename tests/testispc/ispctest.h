#pragma once
//------------------------------------------------------------------------------
/** 
    @class Test::ISPCTest
        
    (C) 2024 Individual contributors, see AUTHORS file
*/    
#include "stdneb.h"
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class ISPCTest : public TestCase
{
    __DeclareClass(ISPCTest);
public:
    /// run the test
    virtual void Run();
};

};
//------------------------------------------------------------------------------ 