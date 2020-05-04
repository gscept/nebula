//------------------------------------------------------------------------------
//  testmem/main.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/appentry.h"
#include "core/coreserver.h"
#include "testbase/testrunner.h"
#include "rendertest.h"

using namespace Core;
using namespace Test;

ImplementNebulaApplication();

void
NebulaMain(const Util::CommandLineArgs& args)
{
	// create Nebula runtime
	Ptr<CoreServer> coreServer = CoreServer::Create();
	coreServer->SetAppName(Util::StringAtom("Nebula Render Tests"));
	coreServer->Open();

	n_printf("NEBULA RENDER TESTS\n");
	n_printf("========================\n");

	// setup and run test runner
	Ptr<TestRunner> testRunner = TestRunner::Create();
	testRunner->AttachTestCase(RenderTest::Create());
	testRunner->Run();
	//testRunner->AttachTestCase(BXmlReaderTest::Create());

	coreServer->Close();
	coreServer = nullptr;
	testRunner = nullptr;

	Core::SysFunc::Exit(0);
}
