//------------------------------------------------------------------------------
//  main.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/appentry.h"
#include "testbase/testrunner.h"
#include "appgame/gameapplication.h"
#include "runtimemoduletest.h"

ImplementNebulaApplication();

class RuntimeModuleTestApp : public App::GameApplication
{
private:
    void SetupGameFeatures() override
    {
        // No additional features required for module loader tests.
    }

    void CleanupGameFeatures() override
    {
        // No-op
    }
};

void
NebulaMain(const Util::CommandLineArgs& args)
{
    RuntimeModuleTestApp app;
    app.SetCompanyName("Test Company");
    app.SetAppTitle("NEBULA RUNTIME MODULE LOADER TESTS");
    app.SetCmdLineArgs(args);

    if (!app.Open())
    {
        n_printf("Aborting runtime module loader tests due to startup failure...\n");
        Core::SysFunc::Exit(-1);
        return;
    }

    n_printf("NEBULA RUNTIME MODULE LOADER TESTS\n");
    n_printf("========================\n");

    Ptr<Test::TestRunner> testRunner = Test::TestRunner::Create();
    testRunner->AttachTestCase(Test::RuntimeModuleLoaderTest::Create());

    bool result = testRunner->Run();

    testRunner = nullptr;
    app.Close();

    Core::SysFunc::Exit(result ? 0 : -1);
}
