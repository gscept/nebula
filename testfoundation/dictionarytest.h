#ifndef TEST_DICTIONARYTEST_H
#define TEST_DICTIONARYTEST_H
//------------------------------------------------------------------------------
/**
    @class Test::Dictionary
    
    Test Util::Dictionary functionality.
    
    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class DictionaryTest : public TestCase
{
    __DeclareClass(DictionaryTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
    