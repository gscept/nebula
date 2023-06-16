#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::PinnedArrayTest

    Tests Nebula's pinned array class.

    (C) 2006 Radon Labs GmbH
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class PinnedArrayTest : public TestCase
{
    __DeclareClass(PinnedArrayTest);
public:
    /// run the test
    virtual void Run();
};

}; // namespace Test
//------------------------------------------------------------------------------
