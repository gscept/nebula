#ifndef TEST_TESTRUNNER_H
#define TEST_TESTRUNNER_H
//------------------------------------------------------------------------------
/**
    @class Test::TestRunner
    
    The test runner class which runs all test cases.
    
    (C) 2006 Radon Labs GmbH
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "util/array.h"
#include "testbase/testcase.h"

//------------------------------------------------------------------------------
namespace Test
{
class TestRunner : public Core::RefCounted
{
    __DeclareClass(TestRunner);
public:
    /// attach a test
    void AttachTestCase(TestCase* testCase);
    /// run the tests
    void Run();

private:
    Util::Array<Ptr<TestCase> > testCases;
};

};    
//------------------------------------------------------------------------------
#endif