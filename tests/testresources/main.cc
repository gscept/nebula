//------------------------------------------------------------------------------
//  testmem/main.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/appentry.h"
#include "core/coreserver.h"
#include "testbase/testrunner.h"
#include "resourcetest.h"

using namespace Core;
using namespace Test;

ImplementNebulaApplication();

void
NebulaMain(const Util::CommandLineArgs& args)
{
    // create Nebula runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName(Util::StringAtom("Nebula Resource Loading Tests"));
    coreServer->Open();

    //Ptr<AssignRegistry> assignReg = AssignRegistry::Create();

    n_printf("\n\nNEBULA RESOURCE TESTS\n");
    n_printf("========================\n");

    // setup and run test runner
    Ptr<TestRunner> testRunner = TestRunner::Create();
    testRunner->AttachTestCase(ResourceTest::Create());
    testRunner->Run();
    //testRunner->AttachTestCase(BXmlReaderTest::Create());

    coreServer->Close();
    coreServer = nullptr;
    testRunner = nullptr;

    Core::SysFunc::Exit(0);
}
