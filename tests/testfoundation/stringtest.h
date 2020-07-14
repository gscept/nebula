#ifndef TEST_STRINGTEST_H
#define TEST_STRINGTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::StringTest
    
    Test functionality of string class.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class StringTest : public TestCase
{
    __DeclareClass(StringTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif    