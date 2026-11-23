//------------------------------------------------------------------------------
//  testneweditor/main.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "foundation.h"
#include "core/coreserver.h"
#include "core/sysfunc.h"
#include "testbase/testrunner.h"
#include "scenedocumenttest.h"

using namespace Core;
using namespace Test;

int
__cdecl main(int argc, const char** argv)
{
    // create Nebula runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName(Util::StringAtom("Nebula New Editor Tests"));
    coreServer->Open();

    n_printf("NEBULA NEW EDITOR TESTS\n");
    n_printf("=======================\n");

    Util::CommandLineArgs args = Util::CommandLineArgs(argc, argv);

    // setup and run test runner
    Ptr<TestRunner> testRunner = TestRunner::Create();
    testRunner->ParseCommandLineArgs(args);
    testRunner->AttachTestCase(SceneDocumentRefTest::Create());
    bool result = testRunner->Run();

    coreServer->Close();
    testRunner = nullptr;
    coreServer = nullptr;

    Core::SysFunc::Exit(result ? 0 : -1);
}
