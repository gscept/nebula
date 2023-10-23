//------------------------------------------------------------------------------
//  testgame/main.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/appentry.h"
#include "testbase/testrunner.h"
#include "appgame/gameapplication.h"
#include "nsharp/scriptfeatureunit.h"

// tests
#include "nsharptest.h"

ImplementNebulaApplication();

using namespace Core;
using namespace Test;

class GameAppTest : public App::GameApplication
{
private:
    /// setup game features
    void
    SetupGameFeatures() override
    {
        scriptFeature = Scripting::ScriptFeatureUnit::Create();
        scriptFeature->SetScriptAssembly("bin:NSharpTests.dll");
        Game::GameServer::Instance()->AttachGameFeature(scriptFeature);
    }

    /// cleanup game features
    void
    CleanupGameFeatures() override
    {
        Game::GameServer::Instance()->RemoveGameFeature(scriptFeature);
        scriptFeature = nullptr;
    }

    Ptr<Scripting::ScriptFeatureUnit> scriptFeature;
};

//------------------------------------------------------------------------------
/**
*/
void
NebulaMain(const Util::CommandLineArgs& args)
{
    GameAppTest gameApp;
    gameApp.SetCompanyName("Test Company");
    gameApp.SetAppTitle("NEBULA NSHARP-TESTS");

    if (!gameApp.Open())
    {
        n_printf("Aborting game system test due to unrecoverable error...\n");
        return;
    }

    n_printf("NEBULA NSHARP-TESTS\n");
    n_printf("========================\n");

    Ptr<TestRunner> testRunner = TestRunner::Create();
    Ptr<NSharpTest> test = NSharpTest::Create();
    test->StepFrame = [&gameApp]() { gameApp.StepFrame(); };
    testRunner->AttachTestCase(test);

    bool result = testRunner->Run();

    testRunner = nullptr;

    gameApp.Close();

    Core::SysFunc::Exit(result ? 0 : -1);
}
