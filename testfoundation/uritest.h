#ifndef TEST_URITEST_H
#define TEST_URITEST_H
//------------------------------------------------------------------------------
/**
    @class Test::URITest
    
    Tests Nebula3's IO::URI class.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class URITest : public TestCase
{
    __DeclareClass(URITest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
