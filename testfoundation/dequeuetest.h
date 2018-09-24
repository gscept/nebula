#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::QueueTest

    Test Nebula's Dequeue functionality.

    (C) 2018 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class DequeueTest : public TestCase
{
    __DeclareClass(DequeueTest);
public:
    /// run the test
    virtual void Run();
};

};
//------------------------------------------------------------------------------
