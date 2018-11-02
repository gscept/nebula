#pragma once
//------------------------------------------------------------------------------
/**
    @class Test::QueueTest

    Test Nebula's Queue functionality.

    (C) 2018 Individual contributors, see AUTHORS file
*/
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class QueueTest : public TestCase
{
    __DeclareClass(QueueTest);
public:
    /// run the test
    virtual void Run();
};

};
//------------------------------------------------------------------------------
