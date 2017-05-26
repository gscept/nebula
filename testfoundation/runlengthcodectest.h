#pragma once
#ifndef TEST_RUNLENGTHCODECTEST_H
#define TEST_RUNLENGTHCODECTEST_H
//------------------------------------------------------------------------------
/** 
    @class Test::RunLengthCodecTest
  
    Test Util::RunLengthCodec functionality.
    http://www.arturocampos.com/ac_rle.html
    
    (C) 2008 Radon Labs GmbH
*/    
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class RunLengthCodecTest : public TestCase
{
    __DeclareClass(RunLengthCodecTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
