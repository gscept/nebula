//------------------------------------------------------------------------------
//  @file testphysics/main.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/appentry.h"
#include "core/coreserver.h"
#include "testbase/testrunner.h"
#include "testbase/stackdebug.h"

// tests
#include "cookingtest.h"

ImplementNebulaApplication();

using namespace Core;
using namespace Test;

//------------------------------------------------------------------------------
/**
    Headless test which runs the physics cooking layer without a renderer and
    without reading any asset files.
*/
void
NebulaMain(const Util::CommandLineArgs& args)
{
    STACK_CHECKPOINT("NebulaMain");

    // create Nebula runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName(Util::StringAtom("Nebula Physics Tests"));
    coreServer->Open();

    n_printf("NEBULA PHYSICS TESTS\n");
    n_printf("====================\n");

    // setup and run test runner
    Ptr<TestRunner> testRunner = TestRunner::Create();
    testRunner->AttachTestCase(CookingTest::Create());
    testRunner->SetVerbose(true);

    bool result = testRunner->Run();

    coreServer->Close();
    testRunner = nullptr;
    coreServer = nullptr;

    Core::SysFunc::Exit(result ? 0 : -1);
}
