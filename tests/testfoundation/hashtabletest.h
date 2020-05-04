#ifndef TEST_HASHTABLETEST_H
#define TEST_HASHTABLETEST_H
//------------------------------------------------------------------------------
/**
    @class Test::HashTableTest
    
    Test HashTable functionality.

    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class HashTableTest : public TestCase
{
    __DeclareClass(HashTableTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
#endif        
    