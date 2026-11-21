//------------------------------------------------------------------------------
//  testgame/main.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/appentry.h"
#include "testbase/testrunner.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "appgame/gameapplication.h"
#include "physicsfeature/physicsfeatureunit.h"
#include "navigationfeature/navigationfeatureunit.h"

// tests
#include "idtest.h"
#include "databasetest.h"
#include "entitysystemtest.h"
#include "hierarchytransformtest.h"
#include "physicstest.h"
#include "navigationtest.h"

#include "testcomponents.h"

ImplementNebulaApplication();

using namespace Core;
using namespace Test;

void
InitializeTestResource(Game::World* world, Game::Entity entity, TestResource* testResource)
{
    n_printf("Original resource: '%s'\n", testResource->resource.Value());
    testResource->resource = "gnyrf.res";
    n_printf("Changed resource to: '%s'\n", testResource->resource.Value());
}

void
InitializeTestVec4(Game::World* world, Game::Entity entity, TestVec4* testVec)
{
    testVec->v4 = Math::vec4(123, 123, 123, 123);
}

namespace Test
{
    ComponentDecayRecord componentDecay;
}

// the decay callback of DecayTestComponent, see Test::EntitySystemTest. It runs outside of the frame
// pipeline while the component is still in its table, which is what allows it to read the data and
// touch anything that belongs to the entity
void
DecayTestComponentDecay(Game::World* world, Game::Entity entity, Test::DecayTestComponent* component)
{
    ++Test::componentDecay.calls;
    Test::componentDecay.lastEntity = entity;

    // writing through the pointer has to be visible from the table, which is what makes the pointer
    // usable for cleaning up anything related to the component
    component->destroyed = true;
    if (world->HasComponent<Test::DecayTestComponent>(entity) && world->GetComponent<Test::DecayTestComponent>(entity).destroyed)
        ++Test::componentDecay.liveReads;
}

class GameAppTest : public App::GameApplication
{
private:
    /// setup game features
    void SetupGameFeatures()
    {
        this->gameFeature = BaseGameFeature::BaseGameFeatureUnit::Instance();

        this->gameFeature->RegisterComponentType<TestResource>({
            .decay = true,
            .OnInit = &InitializeTestResource
        });

        this->gameFeature->RegisterComponentType<TestVec4>({
            .decay = true,
            .OnInit = &InitializeTestVec4
        });
        
        this->gameFeature->RegisterComponentType<TestStruct>();
        this->gameFeature->RegisterComponentType<TestHealth>();
        this->gameFeature->RegisterComponentType<MyFlag>();
        this->gameFeature->RegisterComponentType<TestEmptyStruct>();
        this->gameFeature->RegisterComponentType<TestAsyncComponent>();
        this->gameFeature->RegisterComponentType<DecayTestComponent>({.decay = true, .OnDecay = &DecayTestComponentDecay});

        // the physics feature drives actors and shapes from the component inits, pings and decay
        // callbacks of the entities which the physics tests set up
        this->physicsFeature = PhysicsFeature::PhysicsFeatureUnit::Create();
        this->physicsFeature->SetCmdLineArgs(this->GetCmdLineArgs());
        this->gameServer->AttachGameFeature(this->physicsFeature);

        // the navigation feature loads the navmeshes of NavMesh components
        this->navigationFeature = NavigationFeature::NavigationFeatureUnit::Create();
        this->gameServer->AttachGameFeature(this->navigationFeature);
    }

    /// cleanup game features
    void CleanupGameFeatures()
    {
        // empty
    }

    Ptr<BaseGameFeature::BaseGameFeatureUnit> gameFeature;
    Ptr<PhysicsFeature::PhysicsFeatureUnit> physicsFeature;
    Ptr<NavigationFeature::NavigationFeatureUnit> navigationFeature;
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
    testRunner->AttachTestCase(HierarchyTransformTest::Create());
    testRunner->AttachTestCase(PhysicsManagerTest::Create());
    testRunner->AttachTestCase(NavigationManagerTest::Create());
    
    bool result = testRunner->Run(); 

    testRunner = nullptr;

    gameApp.Close();

    Core::SysFunc::Exit(result?0:-1);
}
