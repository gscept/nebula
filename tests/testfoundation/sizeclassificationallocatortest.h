#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::SizeClassificationAllocatorTest

    Tests Nebula's pinned array class.

    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class SizeClassificationAllocatorTest : public TestCase
{
    __DeclareClass(SizeClassificationAllocatorTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
