#ifndef TEST_GUIDTEST_H
#define TEST_GUIDTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::GuidTest
    
    Test Util::Guid functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class GuidTest : public TestCase
{
    __DeclareClass(GuidTest);
public:
    /// run the test
    virtual void Run();
};

};
//------------------------------------------------------------------------------
#endif    