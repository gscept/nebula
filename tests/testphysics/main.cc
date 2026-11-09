//------------------------------------------------------------------------------
//  @file testphysics/main.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/coreserver.h"
#include "core/sysfunc.h"
#include "testbase/testrunner.h"
#include "testbase/stackdebug.h"

// tests
#include "cookingtest.h"

using namespace Core;
using namespace Test;

//------------------------------------------------------------------------------
/**
    Headless test which runs the physics cooking layer without a renderer and
    without reading any asset files.
*/
int
main(int argc, char** argv)
{
    STACK_CHECKPOINT("main");

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
    return result ? 0 : -1;
}
