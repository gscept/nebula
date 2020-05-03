//------------------------------------------------------------------------------
//  testgame/main.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/appentry.h"
#include "testbase/testrunner.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "appgame/gameapplication.h"

// tests
#include "idtest.h"
#include "componentdatatest.h"
#include "componentsystemtest.h"
#include "loadertest.h"
#include "messagetest.h"
#include "scriptingtest.h"

ImplementNebulaApplication();

using namespace Core;
using namespace Test;

class GameAppTest : public App::GameApplication
{
private:
	/// setup game features
	void SetupGameFeatures()
	{
		gameFeature = BaseGameFeature::BaseGameFeatureUnit::Create();
		this->gameServer->AttachGameFeature(gameFeature);
	}
	/// cleanup game features
	void CleanupGameFeatures()
	{
		this->gameServer->RemoveGameFeature(gameFeature);
		gameFeature->CleanupWorld();
		gameFeature->Release();
		gameFeature = nullptr;
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

	if (!gameApp.Open())
	{
		n_printf("Aborting component system test due to unrecoverable error...\n");
		return;
	}
    
	n_printf("NEBULA GAME-TESTS\n");
	n_printf("========================\n");

	// setup and run test runner
    Ptr<TestRunner> testRunner = TestRunner::Create();
    testRunner->AttachTestCase(IdTest::Create());
    
	Ptr<ComponentSystemTest> compSysTest = ComponentSystemTest::Create();
	compSysTest->gameApp = &gameApp;
	testRunner->AttachTestCase(compSysTest); 
	
	testRunner->AttachTestCase(LoaderTest::Create());

	testRunner->AttachTestCase(CompDataTest::Create());
	testRunner->AttachTestCase(MessageTest::Create());
	testRunner->AttachTestCase(ScriptingTest::Create());
	
    bool result = testRunner->Run(); 

    testRunner = nullptr;

    Core::SysFunc::Exit(result?0:-1);
}
