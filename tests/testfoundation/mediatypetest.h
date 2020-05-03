#ifndef TEST_MEDIATYPETEST_H
#define TEST_MEDIATYPETEST_H
//------------------------------------------------------------------------------
/**
    @class Test::MediaTypeTest
    
    Test MediaType functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class MediaTypeTest : public TestCase
{
    __DeclareClass(MediaTypeTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
