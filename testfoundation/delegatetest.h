#pragma once
#if !__PS3__
//------------------------------------------------------------------------------
/**
    @class Test::DelegateTest
    
    Test Util::Delegate functionality.
    
    (C) 2008 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class DelegateTest : public TestCase
{
    __DeclareClass(DelegateTest);
public:
    /// run the test
    virtual void Run();    
};

}
#endif
//------------------------------------------------------------------------------
    