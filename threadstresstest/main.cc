//------------------------------------------------------------------------------
//  threadstresstest/main.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/appentry.h"
#include "core/coreserver.h"
#include "testbase/testrunner.h"
#include "threadstresstest.h"

using namespace Core;
using namespace Test;


void
__cdecl main()
{
	// create Nebula runtime
	Ptr<CoreServer> coreServer = CoreServer::Create();
	coreServer->SetAppName(Util::StringAtom("Nebula Thread Stress Tests"));
	coreServer->Open();

	n_printf("NEBULA JOBS TESTS\n");
	n_printf("========================\n");

	// setup and run test runner
	Ptr<TestRunner> testRunner = TestRunner::Create();
	testRunner->AttachTestCase(ThreadStressTest::Create());
	bool result = testRunner->Run();	

	coreServer->Close();
	coreServer = nullptr;
	testRunner = nullptr;

    Core::SysFunc::Exit(result ? 0 : -1);
}
