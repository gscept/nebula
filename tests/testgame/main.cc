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
#include "idtest.h"
#include "databasetest.h"
#include "entitysystemtest.h"
#include "scriptingtest.h"

#include "testcomponents.h"

ImplementNebulaApplication();

using namespace Core;
using namespace Test;

template<typename T>
void RegisterComponent()
{
    Util::StringAtom const name = T::TableType::Traits::name;
    MemDb::AttributeId const cid = MemDb::TypeRegistry::Register<T>(name, T(), 0);
    //Game::ComponentSerialization::Register<Resources::ResourceName>(cid);
    //Game::ComponentInspection::Register(cid, &Game::ComponentDrawFuncT<Resources::ResourceName>);
}

class GameAppTest : public App::GameApplication
{
private:
    /// setup game features
    void SetupGameFeatures()
    {
        RegisterComponent<TestComponentT>();
        // empty
    }
    /// cleanup game features
    void CleanupGameFeatures()
    {
        // empty
    }

    Ptr<BaseGameFeature::BaseGameFeatureUnit> gameFeature;
};

//------------------------------------------------------------------------------
/**
*/
void
NebulaMain(const Util::CommandLineArgs& args)
{
    GameAppTest gameApp;
    gameApp.SetCompanyName("Test Company");
    gameApp.SetAppTitle("NEBULA GAME-TESTS");

    Game::BlueprintManager::SetBlueprintsFilename("blueprints_test.json", "bin:");

    if (!gameApp.Open())
    {
        n_printf("Aborting game system test due to unrecoverable error...\n");
        return;
    }
    
    n_printf("NEBULA GAME-TESTS\n");
    n_printf("========================\n");

    // setup and run test runner
    Ptr<TestRunner> testRunner = TestRunner::Create();
    testRunner->AttachTestCase(IdTest::Create());
    testRunner->AttachTestCase(DatabaseTest::Create());
    testRunner->AttachTestCase(EntitySystemTest::Create());
    //testRunner->AttachTestCase(ScriptingTest::Create());
    
    bool result = testRunner->Run(); 

    testRunner = nullptr;

    gameApp.Close();

    Core::SysFunc::Exit(result?0:-1);
}
