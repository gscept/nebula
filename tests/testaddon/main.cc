//------------------------------------------------------------------------------
//  testaddon/main.cc
//  (C) 2020 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "foundation.h"
#include "core/coreserver.h"
#include "core/sysfunc.h"
#include "io/gamecontentserver.h"
#include "testbase/testrunner.h"

// tests
#include "databasetest.h"
#include "datasettest.h"
#include "dbattrs.h"

using namespace Core;
using namespace Test;

int
__cdecl main()
{
    // create Nebula runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName(Util::StringAtom("Nebula Addon Tests"));
    coreServer->Open();

    n_printf("NEBULA ADDON TESTS\n");
    n_printf("========================\n");

    // setup and run test runner
    Ptr<TestRunner> testRunner = TestRunner::Create();
    testRunner->AttachTestCase(DatabaseTest::Create());
    testRunner->AttachTestCase(DatasetTest::Create());
    bool result = testRunner->Run();

    coreServer->Close();
    testRunner = nullptr;
    coreServer = nullptr;

    Core::SysFunc::Exit(result ? 0 : -1);
}
