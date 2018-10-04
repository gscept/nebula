#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::ArrayQueueTest

    Test Nebula's ArrayQueue functionality.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class ArrayQueueTest : public TestCase
{
    __DeclareClass(ArrayQueueTest);
public:
    /// run the test
    virtual void Run();
};

};
//------------------------------------------------------------------------------