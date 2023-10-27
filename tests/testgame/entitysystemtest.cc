//------------------------------------------------------------------------------
//  entitysystemtest.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "entitysystemtest.h"
#include "timing/timer.h"
#include "basegamefeature/managers/blueprintmanager.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "framesync/framesynctimer.h"
#include "profiling/profiling.h"
#include "game/gameserver.h"
#include "testcomponents.h"

#include "flatbuffers/idl.h"
#include "flatbuffers/util.h"

#include "basegamefeature/components/basegamefeature.h"

#include "io/textreader.h"
#include "io/filestream.h"

using namespace Game;
using namespace Math;
using namespace Util;

namespace Test
{

static int numFramesExecuted = 0;

//------------------------------------------------------------------------------

__ImplementClass(Test::EntitySystemTest, 'GEST', Test::TestCase);

void
StepFrame()
{
#if NEBULA_ENABLE_PROFILING
    Profiling::ProfilingNewFrame();
#endif
    Game::GameServer::Instance()->OnBeginFrame();
    Game::GameServer::Instance()->OnFrame();
    Game::GameServer::Instance()->OnEndFrame();
}

//------------------------------------------------------------------------------
/**
    @todo   this test should be more thorough and make sure that instances retain
            the correct data after shuffeling around (deletion, creation, defrag etc.)
*/
void
EntitySystemTest::Run()
{    
#if NEBULA_ENABLE_PROFILING
    Profiling::ProfilingRegisterThread();
#endif

    bool setupFrameSync = !FrameSync::FrameSyncTimer::HasInstance();
    Ptr<FrameSync::FrameSyncTimer> t;
    if (setupFrameSync)
    {
        t = FrameSync::FrameSyncTimer::Create();
        t->Setup();
    }
    else
    {
        t = FrameSync::FrameSyncTimer::Instance();
    }

    TemplateId const playerBlueprint = Game::GetTemplateId("Player"_atm);
    TemplateId const enemyBlueprint = Game::GetTemplateId("Enemy"_atm);

    World* world = Game::GetWorld(WORLD_DEFAULT);

    for (int i = 0; i < 500; i++)
    {
        Entity player = world->CreateEntity({playerBlueprint});
    }

    Util::Array<Entity> enemies;
    for (int i = 0; i < 500; i++)
    {
        Entity enemy = world->CreateEntity({enemyBlueprint});
        enemies.Append(enemy);
    }

    StepFrame();

    for (int i = 0; i < 200; i++)
    {
        world->DeleteEntity(enemies[i]);
    }

    int i;
    for (i = 0; i < 100; i++)
    {
        StepFrame();

        if (i % 5 == 1)
        {
            world->DeleteEntity(enemies[i + 200]);
        }
    }

    // Delete all entities
    for (auto e : enemies)
    {
        if (world->IsValid(e))
            world->DeleteEntity(e);
    }

    // Run a frame
    StepFrame();

    Util::Queue<Game::Entity> queue;
    for (int i = 0; i < 10; i++)
    {
        Entity enemy = world->CreateEntity({enemyBlueprint});
        queue.Enqueue(enemy);
    }

    // Run a frame with new entities
    StepFrame();

    // Delete entities from front
    for (int i = 0; i < 5; i++)
    {
        auto e = queue.Dequeue();
        world->DeleteEntity(e);
    }

    // run a frame
    StepFrame();

    // Delete all
    while (!queue.IsEmpty())
    {
        auto e = queue.Dequeue();
        world->DeleteEntity(e);
    }

    // run a frame
    StepFrame();

    Util::Stack<Game::Entity> stack;
    for (int i = 0; i < 10; i++)
    {
        Entity enemy = world->CreateEntity({enemyBlueprint});
        stack.Push(enemy);
    }

    // run a frame
    StepFrame();

    // Delete entities from back
    for (int i = 0; i < 5; i++)
    {
        auto e = stack.Pop();
        world->DeleteEntity(e);
    }

    // run a frame
    StepFrame();

    // Delete all
    while (!stack.IsEmpty())
    {
        auto e = stack.Pop();
        world->DeleteEntity(e);
    }

    // run a frame
    StepFrame();

    Game::Entity entities[10] = {
        world->CreateEntity({playerBlueprint}),
        world->CreateEntity({playerBlueprint}),
        world->CreateEntity({playerBlueprint}),
        world->CreateEntity({playerBlueprint}),
        world->CreateEntity({playerBlueprint}),
        world->CreateEntity({playerBlueprint}),
        world->CreateEntity({playerBlueprint}),
        world->CreateEntity({playerBlueprint}),
        world->CreateEntity({playerBlueprint}),
        world->CreateEntity({playerBlueprint})};

    // run a frame
    StepFrame();

    // Delete the last entity only
    world->DeleteEntity(entities[9]);

    // run a frame
    StepFrame();

    // Delete the last entity, and also another entity. This will cause
    // the entity to swap place with an invalid instance, which SHOULD be OK
    world->DeleteEntity(entities[8]);
    world->DeleteEntity(entities[4]);

    // run a frame
    StepFrame();

    VERIFY(true);

    // Test managed properties
    {
        Game::RegisterComponent<DecayTestComponent>(COMPONENTFLAG_DECAY);
        ComponentId decayComponentId = Game::GetComponentId<DecayTestComponent>();
        
        Game::EntityCreateInfo enemyInfo = {enemyBlueprint, true};
        Game::Entity enemies[] = {
            world->CreateEntity(enemyInfo),
            world->CreateEntity(enemyInfo),
            world->CreateEntity(enemyInfo),
            world->CreateEntity(enemyInfo),
            world->CreateEntity(enemyInfo),
        };

        {
            VERIFY(
                world->GetDatabase()->GetTable(world->GetEntityMapping(enemies[0]).table).GetNumRows() ==
                5
            );
        }

        Game::Op::RegisterComponent regOp;
        regOp.entity = enemies[0];
        regOp.component = decayComponentId ;
        world->Execute(regOp);
        regOp.entity = enemies[1];
        regOp.component = decayComponentId ;
        world->Execute(regOp);
        regOp.entity = enemies[2];
        regOp.component = decayComponentId ;
        world->Execute(regOp);

        {
            VERIFY(
                world->GetDatabase()->GetTable(world->GetEntityMapping(enemies[0]).table).GetNumRows() ==
                3
            );
        }

        StepFrame();

        // delete an entity
        world->DeleteEntity(enemies[2]);

        StepFrame();

        {
            MemDb::TableId tid = world->GetEntityMapping(enemies[0]).table;
            VERIFY(world->GetDatabase()->GetTable(tid).GetNumRows() == 2);
        }

        StepFrame();

        {
            MemDb::TableId tid = world->GetEntityMapping(enemies[0]).table;
            VERIFY(world->GetDatabase()->GetTable(tid).GetNumRows() == 2);
        }
    }

    StepFrame();

    {
        Game::FilterBuilder::FilterCreateInfo filterInfo;
        filterInfo.inclusive[0] = Game::GetComponentId("TestHealth");
        filterInfo.access[0] = Game::AccessMode::READ;
        filterInfo.inclusive[1] = Game::GetComponentId("TestStruct");
        filterInfo.access[1] = Game::AccessMode::WRITE;
        filterInfo.numInclusive = 2;
        Game::Filter filter = Game::FilterBuilder::CreateFilter(filterInfo);

        Game::Dataset set = world->Query(filter);

        for (int v = 0; v < set.numViews; v++)
        {
            Game::Dataset::EntityTableView const& view = set.views[v];
            TestHealth* healths = (TestHealth*)view.buffers[0];
            TestStruct* strs = (TestStruct*)view.buffers[1];

            for (IndexT i = 0; i < view.numInstances; ++i)
            {
                TestHealth& h = healths[i];
                TestStruct& s = strs[i];

                VERIFY(s.foo == 1);
                s.foo = h.value;
            }
        }

        Game::DestroyFilter(filter);
    }

    Game::Filter filter = Game::FilterBuilder().Including<const TestHealth, const TestStruct>().Build();

    Game::Dataset set = world->Query(filter);

    for (int v = 0; v < set.numViews; v++)
    {
        Game::Dataset::EntityTableView const& view = set.views[v];
        TestHealth* healths = (TestHealth*)view.buffers[0];
        TestStruct* strs = (TestStruct*)view.buffers[1];

        for (IndexT i = 0; i < view.numInstances; ++i)
        {
            TestHealth& h = healths[i];
            TestStruct& s = strs[i];

            VERIFY(s.foo == h.value);
        }
    }

    VERIFY(set.numViews > 3);

    Test::TestHealth* healths = (Test::TestHealth*)set.views[0].buffers[0];
    Test::TestStruct* structs = (Test::TestStruct*)set.views[0].buffers[1];

    // add a component to an entity that does not already have it. This should
    // move the entity from one table to another, and in this case
    // creating a new table that contains only one instance (this one)
    world->AddComponent<TestVec4>(entities[1], nullptr);

    Game::DestroyFilter(filter);
    Game::ReleaseDatasets();

    StepFrame();

    
    bool hasExecutedUpdateFunc = false;
    std::function updateFunc = [&](World* world, Test::TestHealth const& testHealth, Test::TestStruct& testStruct)
    {
        hasExecutedUpdateFunc = true;
    };

    Game::ProcessorBuilder("TestUpdateFunc").Func(updateFunc).Including<TestVec4>().Build();

    StepFrame();

    // make sure update func is actually executed
    VERIFY(hasExecutedUpdateFunc);


    // Test on activate func
    // Create one entity, which will go through the on activate step, once and once only.
    Game::EntityCreateInfo enemyInfo = {enemyBlueprint, true};
    world->CreateEntity(enemyInfo);

    int numActivateExecutions = 0;
    std::function activateFunc = [&](World* world, Test::TestHealth const& testHealth, Test::TestStruct& testStruct)
    { 
        numActivateExecutions++;
    };

    Game::ProcessorBuilder("TestActivateFunc").On("OnActivate").Func(activateFunc).Build();

    StepFrame();
    VERIFY(numActivateExecutions == 1);

    // Doublecheck so that we don't execute the activate func twice for the same entity
    StepFrame();
    VERIFY(numActivateExecutions == 1);

    t->StopTime();
}

} // namespace Test