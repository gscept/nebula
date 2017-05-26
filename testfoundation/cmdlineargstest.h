#ifndef TEST_CMDLINEARGSTEST_H
#define TEST_CMDLINEARGSTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::CmdLineArgsTest
    
    Test Util::CmdLineArgs test.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class CmdLineArgsTest : public TestCase
{
    __DeclareClass(CmdLineArgsTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
    