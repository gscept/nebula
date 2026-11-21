//------------------------------------------------------------------------------
//  @file testnavigation/main.cc
//  @copyright (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "core/coreserver.h"
#include "core/sysfunc.h"
#include "io/ioserver.h"
#include "testbase/testrunner.h"
#include "testbase/stackdebug.h"
#include "nflatbuffer/flatbufferinterface.h"

// tests
#include "navbuildtest.h"
#include "navassettest.h"

using namespace Core;
using namespace Test;

//------------------------------------------------------------------------------
/**
    Headless test which builds navmeshes from generated geometry, without a
    renderer and without reading any asset files.
*/
int
main(int argc, char** argv)
{
    STACK_CHECKPOINT("main");

    // create Nebula runtime
    Ptr<CoreServer> coreServer = CoreServer::Create();
    coreServer->SetAppName(Util::StringAtom("Nebula Navigation Tests"));
    coreServer->Open();

    // the asset tests write and read files
    IO::StartupFilesystem({});
    // the area tables are compiled from json with the flatbuffer schemas. A test of the engine has no project
    // export, the schema of the area table is the one the engine exports for itself
    Flat::FlatbufferInterface::Init();
#ifdef NEBULA_TOOLKIT_ROOT
    if (!Flat::FlatbufferInterface::HasSchema("NAVR"))
    {
        Flat::FlatbufferInterface::LoadSchema(IO::URI(Util::String(NEBULA_TOOLKIT_ROOT) + "/syswork/export/data/flatbuffer/navigation/navareas.bfbs"));
    }
#endif

    n_printf("NEBULA NAVIGATION TESTS\n");
    n_printf("=======================\n");

    // setup and run test runner
    Ptr<TestRunner> testRunner = TestRunner::Create();
    testRunner->AttachTestCase(NavBuildTest::Create());
    testRunner->AttachTestCase(NavAssetTest::Create());
    testRunner->SetVerbose(true);

    bool result = testRunner->Run();

    testRunner = nullptr;
    IO::ShutdownFilesystem();
    coreServer->Close();
    coreServer = nullptr;

    Core::SysFunc::Exit(result ? 0 : -1);
    return result ? 0 : -1;
}
