#pragma once
#ifndef TEST_WIN32REGISTRYTEST_H
#define TEST_WIN32REGISTRYTEST_H
//------------------------------------------------------------------------------
/** 
    @class Test::Win32RegistryTest
  
    Test Win32Registry functionality.
    
    (C) 2007 Radon Labs GmbH
*/    
#include "stdneb.h"
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class Win32RegistryTest : public TestCase
{
    __DeclareClass(Win32RegistryTest);
public:
    /// run the test
    virtual void Run();
};

};
//------------------------------------------------------------------------------
#endif    