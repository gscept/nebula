//------------------------------------------------------------------------------
//  testispc/main.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "foundation.h"
#include "core/coreserver.h"
#include "testbase/testrunner.h"
#include "ispctest.h"


using namespace Core;
using namespace Test;

int main(int argc, char** argv)
{
    // create Nebula runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName("Nebula flatc Tests");
    coreServer->Open();

    n_printf("NEBULA ispc TESTS\n");
    n_printf("========================\n");

    // setup and run test runner
    Ptr<TestRunner> testRunner = TestRunner::Create();
    testRunner->AttachTestCase(ISPCTest::Create());    
    testRunner->Run(); 

    coreServer->Close();
    testRunner = nullptr;
    coreServer = nullptr;
    
    Core::SysFunc::Exit(0);
}
