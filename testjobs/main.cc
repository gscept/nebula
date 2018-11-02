//------------------------------------------------------------------------------
//  testmem/main.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/appentry.h"
#include "core/coreserver.h"
#include "testbase/testrunner.h"
#include "jobstest.h"

using namespace Core;
using namespace Test;


void
__cdecl main()
{
	// create Nebula runtime
	Ptr<CoreServer> coreServer = CoreServer::Create();
	coreServer->SetAppName(Util::StringAtom("Nebula Jobs Tests"));
	coreServer->Open();

	n_printf("NEBULA JOBS TESTS\n");
	n_printf("========================\n");

	// setup and run test runner
	Ptr<TestRunner> testRunner = TestRunner::Create();
	testRunner->AttachTestCase(JobsTest::Create());
	testRunner->Run();	

	coreServer->Close();
	coreServer = nullptr;
	testRunner = nullptr;

	Core::SysFunc::Exit(0);
}
