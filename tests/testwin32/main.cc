//------------------------------------------------------------------------------
//  testwin32/main.cc
//  (C) 2012-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "foundation.h"
#include "core/coreserver.h"
#include "core/sysfunc.h"
#include "testbase/testrunner.h"
#include "testwin32/win32registrytest.h"

using namespace Core;
using namespace Test;

int
main(int argc, char** argv)
{
    // create Nebula runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName("Nebula win32 Tests");
    coreServer->Open();

    n_printf("NEBULA win32 TESTS\n");
    n_printf("========================\n");

    // setup and run test runner
    Ptr<TestRunner> testRunner = TestRunner::Create();
    testRunner->AttachTestCase(Win32RegistryTest::Create());    
    bool result = testRunner->Run(); 

    coreServer->Close();
    testRunner = nullptr;
    coreServer = nullptr;
    
    Core::SysFunc::Exit(result ? 0 : -1);
}
