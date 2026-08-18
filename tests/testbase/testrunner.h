#ifndef TEST_TESTRUNNER_H
#define TEST_TESTRUNNER_H
//------------------------------------------------------------------------------
/**
    @class Test::TestRunner
    
    The test runner class which runs all test cases.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/ptr.h"
#include "util/array.h"
#include "testbase/testcase.h"
#include "util/commandlineargs.h"

//------------------------------------------------------------------------------
namespace Test
{
class TestRunner : public Core::RefCounted
{
    __DeclareClass(TestRunner);
public:
    /// attach a test
    void AttachTestCase(TestCase* testCase);
    /// set verbose output
    void SetVerbose(bool verbose);
    /// parse command line arguments
    void ParseCommandLineArgs(Util::CommandLineArgs const& args);
    /// run the tests
    bool Run();

private:
    Util::Array<Ptr<TestCase>> testCases;
    bool verbose = false;
};

//--------------------------------------------------------------------------
/**
*/
inline void
TestRunner::SetVerbose(bool verbose)
{
    this->verbose = verbose;
}

};    
//------------------------------------------------------------------------------
#endif