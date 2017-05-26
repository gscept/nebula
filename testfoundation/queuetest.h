#ifndef TESTS_QUEUETEST_H
#define TESTS_QUEUETEST_H
//------------------------------------------------------------------------------
/**
    @class Test::QueueTest

    Test Nebula3's Queue functionality.

    (C) 2006 Radon Labs GmbH
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
#endif    