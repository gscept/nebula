//------------------------------------------------------------------------------
//  testgame/main.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/appentry.h"
#include "testbase/testrunner.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "appgame/gameapplication.h"
#include "basegamefeature/managers/blueprintmanager.h"

// tests
#include "nsharptest.h"

ImplementNebulaApplication();

using namespace Core;
using namespace Test;

//------------------------------------------------------------------------------
/**
*/
void
NebulaMain(const Util::CommandLineArgs& args)
{
    // create Nebula runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName(Util::StringAtom("Nebula NSharp tests"));
    coreServer->Open();

    Ptr<IO::IoServer> ioServer = IO::IoServer::Create();
    ioServer->MountStandardArchives();
    Ptr<IO::IoInterface> ioInterface = IO::IoInterface::Create();
    ioInterface->Open();

    n_printf("NEBULA NSHARP-TESTS\n");
    n_printf("========================\n");

    Ptr<TestRunner> testRunner = TestRunner::Create();
    testRunner->AttachTestCase(NSharpTest::Create());
    
    bool result = testRunner->Run(); 

    testRunner = nullptr;

    Core::SysFunc::Exit(result?0:-1);
}
