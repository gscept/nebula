#pragma once
//------------------------------------------------------------------------------
/** 
    @class Test::FlatCTest
        
    (C) 2018 Individual contributors, see AUTHORS file
*/    
#include "stdneb.h"
#include "testbase/testcase.h"
#include "flatbuffers/flatbuffers.h"

//------------------------------------------------------------------------------
namespace Test
{
class FlatCTest : public TestCase
{
    __DeclareClass(FlatCTest);
public:
    /// run the test
    virtual void Run();
};

};
//------------------------------------------------------------------------------ 