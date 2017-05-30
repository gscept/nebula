//------------------------------------------------------------------------------
//  ps3mathtest_ps3/main.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/appentry.h"
#include "core/coreserver.h"
#include "testbase/testrunner.h"

// tests
#include "scalartest.h"
#include "float4test.h"
#include "matrix44test.h"
#include "transformtest.h"
#include "quaterniontest.h"
#include "vectortest.h"
#include "pointtest.h"
#include "planetest.h"
#include "testbase/stackdebug.h"

/*
#if __PS3__
#include <sys/process.h>
// change stacksize from default 64kB to 96kB
SYS_PROCESS_PARAM (1001, 0x18000) // (priority, stack size)
#endif
*/

ImplementNebulaApplication();

using namespace Core;
using namespace Test;

//------------------------------------------------------------------------------
/**
*/
void
NebulaMain(const Util::CommandLineArgs& args)
{
    STACK_CHECKPOINT("NebulaMain");

    // create Nebula3 runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName(Util::StringAtom("Nebula3 Foundation Math-Tests"));
    coreServer->Open();

    n_printf("NEBULA3 FOUNDATION MATH-TESTS\n");
    n_printf("========================\n");

	// setup and run test runner
    Ptr<TestRunner> testRunner = TestRunner::Create();
    testRunner->AttachTestCase(ScalarTest::Create());
    testRunner->AttachTestCase(Float4Test::Create());
	testRunner->AttachTestCase(Matrix44Test::Create());
    testRunner->AttachTestCase(TransformTest::Create());
	testRunner->AttachTestCase(QuaternionTest::Create());
	testRunner->AttachTestCase(VectorTest::Create());
	testRunner->AttachTestCase(PointTest::Create());
    testRunner->AttachTestCase(PlaneTest::Create());

    testRunner->Run(); 

    coreServer->Close();
    testRunner = 0;
    coreServer = 0;
    
    Core::SysFunc::Exit(0);
}
