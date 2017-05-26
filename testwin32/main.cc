//------------------------------------------------------------------------------
//  testwin32/main.cc
//  (C) 2012 gscept
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "foundation.h"
#include "core/coreserver.h"
#include "core/sysfunc.h"
#include "testbase/testrunner.h"
#include "testwin32/win32registrytest.h"

using namespace Core;
using namespace Test;

void
__cdecl main()
{
    // create Nebula3 runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName("Nebula3 win32 Tests");
    coreServer->Open();

    n_printf("NEBULA3 win32 TESTS\n");
    n_printf("========================\n");

    // setup and run test runner
    Ptr<TestRunner> testRunner = TestRunner::Create();
    testRunner->AttachTestCase(Win32RegistryTest::Create());    
    testRunner->Run(); 

    coreServer->Close();
    testRunner = 0;
    coreServer = 0;
    
    Core::SysFunc::Exit(0);
}
