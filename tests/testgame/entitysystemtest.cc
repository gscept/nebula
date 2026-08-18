//------------------------------------------------------------------------------
//  entitysystemtest.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "entitysystemtest.h"
#include "basegamefeature/level.h"
#include "framesync/framesynctimer.h"
#include "profiling/profiling.h"
#include "game/gameserver.h"
#include "testcomponents.h"
#include "io/ioserver.h"
#include "basegamefeature/components/position.h"
#include "basegamefeature/components/orientation.h"
#include "basegamefeature/components/scale.h"

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

//--------------------------------------------------------------------------
/**
*/
Game::Entity CreatePlayerEntity(Game::World* world)
{
    Game::Entity entity = world->CreateEntity();
    world->AddComponent<TestHealth>(entity);
    world->AddComponent<TestStruct>(entity);
    world->AddComponent<TestEmptyStruct>(entity);
    return entity;
}

//--------------------------------------------------------------------------
/**
*/
Game::Entity CreateEnemyEntity(Game::World* world)
{
    Game::Entity entity = world->CreateEntity();
    world->AddComponent<TestHealth>(entity);
    world->AddComponent<TestVec4>(entity);
    world->AddComponent<TestStruct>(entity);
    return entity;
}

//--------------------------------------------------------------------------
/**
*/
Game::Entity CreateAsyncTestEntity(Game::World* world)
{
    Game::Entity entity = world->CreateEntity();
    world->AddComponent<TestHealth>(entity);
    world->AddComponent<TestStruct>(entity);
    world->AddComponent<TestEmptyStruct>(entity);
    world->AddComponent<TestAsyncComponent>(entity);
    return entity;
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

    World* world = Game::GetWorld(WORLD_DEFAULT);

    for (int i = 0; i < 500; i++)
    {
        Entity player = CreatePlayerEntity(world);
    }

    Util::Array<Entity> enemies;
    for (int i = 0; i < 500; i++)
    {
        Entity enemy = CreateEnemyEntity(world);
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
        Entity enemy = CreateEnemyEntity(world);
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
        Entity enemy = CreateEnemyEntity(world);
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
        CreatePlayerEntity(world),
        CreatePlayerEntity(world),
        CreatePlayerEntity(world),
        CreatePlayerEntity(world),
        CreatePlayerEntity(world),
        CreatePlayerEntity(world),
        CreatePlayerEntity(world),
        CreatePlayerEntity(world),
        CreatePlayerEntity(world),
        CreatePlayerEntity(world)
    };

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
        ComponentId decayComponentId = Game::GetComponentId<DecayTestComponent>();
        
        Game::Entity enemies[] = {
            CreateEnemyEntity(world),
            CreateEnemyEntity(world),
            CreateEnemyEntity(world),
            CreateEnemyEntity(world),
            CreateEnemyEntity(world),
        };

        // All entities should have these components
        VERIFY(world->HasComponent<Game::Entity>(enemies[0]));
        VERIFY(world->HasComponent<Game::Position>(enemies[0]));
        VERIFY(world->HasComponent<Game::Orientation>(enemies[0]));
        VERIFY(world->HasComponent<Game::Scale>(enemies[0]));

        {
            VERIFY(
                world->GetDatabase()->GetTable(world->GetEntityMapping(enemies[0]).table).GetNumRows() ==
                5
            );
        }

        world->AddComponent<DecayTestComponent>(enemies[0]);
        world->AddComponent<DecayTestComponent>(enemies[1]);
        world->AddComponent<DecayTestComponent>(enemies[2]);

        // add components are always delayed, thus step one frame to make sure they're created properly and the entity has been migrated
        StepFrame();

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
            Game::Dataset::View const& view = set.views[v];
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
        Game::Dataset::View const& view = set.views[v];
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
    world->AddComponent<TestVec4>(entities[1]);

    Game::DestroyFilter(filter);
    Game::ReleaseDatasets();

    StepFrame();

    {
        Entity entity = CreatePlayerEntity(world);

        Game::Position pos = world->GetComponent<Game::Position>(entity);
        Game::Orientation orient = world->GetComponent<Game::Orientation>(entity);
        Game::Scale scale = world->GetComponent<Game::Scale>(entity);
        
        Game::Position pos2 = pos + vec3(1, 1, 1);
        Game::Orientation orient2 = orient * quat(2, 2, 2, 2);
        Game::Scale scale2 = scale + vec3(1, 1, 1);

        world->SetComponent(entity, pos2);
        world->SetComponent(entity, orient2);
        world->SetComponent(entity, scale2);

        // Verify that set and get is actually working
        VERIFY(world->GetComponent<Game::Position>(entity) != pos);
        VERIFY(world->GetComponent<Game::Position>(entity) == pos2);

        VERIFY(world->GetComponent<Game::Orientation>(entity) != orient);
        VERIFY(world->GetComponent<Game::Orientation>(entity) == orient2);

        VERIFY(world->GetComponent<Game::Scale>(entity) != scale);
        VERIFY(world->GetComponent<Game::Scale>(entity) == scale2);

        world->AddComponent<TestResource>(entity);
        
        StepFrame();

        TestResource testResource = world->GetComponent<TestResource>(entity);
        TestResource testResource2;
        testResource2.resource = "fjidklshjfkdshjkfds"_atm;
        world->SetComponent<TestResource>(entity, testResource2);

        VERIFY(world->GetComponent<TestResource>(entity).resource != testResource.resource);
        VERIFY(world->GetComponent<TestResource>(entity).resource == testResource2.resource);
    }

    {
        Entity entity = CreatePlayerEntity(world);

        // create a temporary component.
        // these are constructed directly if it has an OnStage delegate.
        // This queues the component in a cmd buffer. all add commands should be sorted before execution, based on which entities they belong to.
        // This way, we can move the entity once, even if it gets multiple components added in a single frame.
        DecayTestComponent* testDecay = world->AddComponent<DecayTestComponent>(entity);

        TestResource* testResource = world->AddComponent<TestResource>(entity);
        // test that the OnInit function is actually executed correctly
        VERIFY(testResource->resource == "gnyrf.res"_atm);

        testResource->resource = "foobar.res"_atm;

        // The component should not be added immediately, but instead it should be staged in a cmd buffer and added later.
        VERIFY(!world->HasComponent<TestResource>(entity));
        VERIFY(!world->HasComponent<DecayTestComponent>(entity));

        StepFrame();
        // The memory of testResource has been freed by the World, thus it's unsafe to use now.
        testResource = nullptr;
        testDecay = nullptr;

        // the components should be added now
        VERIFY(world->HasComponent<TestResource>(entity));
        VERIFY(world->HasComponent<DecayTestComponent>(entity));

        VERIFY(world->GetComponent<TestResource>(entity).resource == "foobar.res"_atm);
    }
    {
        Game::EntityTableCreateInfo tableInfo;
        tableInfo.name = "RawAllocationTest";
        tableInfo.components = Util::FixedArray<Game::ComponentId>(5);
        tableInfo.components[0] = Game::GetComponentId<Game::Entity>();
        tableInfo.components[1] = Game::GetComponentId<Game::Position>();
        tableInfo.components[2] = Game::GetComponentId<Game::Orientation>();
        tableInfo.components[3] = Game::GetComponentId<Game::Scale>();
        tableInfo.components[4] = Game::GetComponentId<TestVec4>();
        MemDb::TableId const table = world->CreateEntityTable(tableInfo);

        Entity const entity = world->AllocateEntityId();
        world->AllocateInstance(entity, table);
        VERIFY(world->GetComponent<TestVec4>(entity).v4 == Math::vec4(1, 2, 3, 4));
        world->InitializeInstance(entity);
        VERIFY(world->GetComponent<TestVec4>(entity).v4 == Math::vec4(123, 123, 123, 123));

        Entity const prefetchedEntity = world->AllocateEntityId();
        MemDb::RowId const prefetchedInstance = world->AllocateInstance(prefetchedEntity, table);
        VERIFY(world->GetComponent<TestVec4>(prefetchedEntity).v4 == Math::vec4(1, 2, 3, 4));
        world->InitializeInstance(prefetchedEntity, table, prefetchedInstance);
        VERIFY(world->GetComponent<TestVec4>(prefetchedEntity).v4 == Math::vec4(123, 123, 123, 123));
    }
    bool hasExecutedUpdateFunc = false;
    std::function updateFunc = [&](World* world, Test::TestHealth const& testHealth, Test::TestStruct& testStruct)
    {
        hasExecutedUpdateFunc = true;
    };

    std::function updateFuncAsync = [](World* world, Test::TestHealth const& testHealth, Test::TestStruct& testStruct, Test::TestAsyncComponent)
    {
        testStruct.enumerable = Test::TestEnumType::Two;
        for (int i = 0; i < 100; i++)
        {
            testStruct.bar = Math::sqrt(10 + testStruct.bar);
        }
    };

    Game::ProcessorBuilder(world, "TestUpdateFunc").Func(updateFunc).Including<TestVec4>().Build();
    
    StepFrame();

    // make sure update func is actually executed
    VERIFY(hasExecutedUpdateFunc);

    // Test async processors
    for (int i = 0; i < 10000; i++)
    {
        CreateAsyncTestEntity(world);
    }

    StepFrame();
    
    Game::ProcessorBuilder(world, "TestUpdateFuncAsync").Func(updateFuncAsync).Async().Build();
    
    StepFrame();

    // Packed levels can be instantiated repeatedly and unloading only releases packed source data.
    Util::String const packedLevelPath = "temp:levels/entitysystemtest.nlvl";
    Game::World sourceWorld(Game::WorldHash('LSRC'), 7);
    Game::Entity sourceEntity = sourceWorld.CreateEntity(true);
    TestHealth* sourceHealth = sourceWorld.AddComponent<TestHealth>(sourceEntity);
    sourceHealth->value = 123;
    TestVec4* sourceVector = sourceWorld.AddComponent<TestVec4>(sourceEntity);
    sourceVector->v4 = Math::vec4(1, 2, 3, 4);
    sourceWorld.ExecuteAddComponentCommands();
    VERIFY(sourceWorld.ExportLevel(packedLevelPath));

    Game::World destinationWorld(Game::WorldHash('LDST'), 8);
    Game::PackedLevel* packedLevel = destinationWorld.PreloadLevel(packedLevelPath);
    VERIFY(packedLevel != nullptr);
    if (packedLevel != nullptr)
    {
        Util::Array<Game::Entity> firstInstance = packedLevel->Instantiate();
        Util::Array<Game::Entity> secondInstance = packedLevel->Instantiate();
        VERIFY(firstInstance.Size() == 1);
        VERIFY(secondInstance.Size() == 1);
        VERIFY(destinationWorld.GetComponent<TestHealth>(firstInstance[0]).value == 123);
        VERIFY(destinationWorld.GetComponent<TestHealth>(secondInstance[0]).value == 123);
        VERIFY(destinationWorld.GetComponent<TestVec4>(firstInstance[0]).v4 == Math::vec4(123, 123, 123, 123));
        VERIFY(destinationWorld.GetComponent<TestVec4>(secondInstance[0]).v4 == Math::vec4(123, 123, 123, 123));

        destinationWorld.UnloadLevel(packedLevel);
        VERIFY(destinationWorld.IsValid(firstInstance[0]));
        VERIFY(destinationWorld.IsValid(secondInstance[0]));
    }

    Game::World stagingWorld(Game::WorldHash('LSTG'), 9);
    stagingWorld.componentInitializationEnabled = false;
    Game::PackedLevel* stagingLevel = stagingWorld.PreloadLevel(packedLevelPath);
    VERIFY(stagingLevel != nullptr);
    if (stagingLevel != nullptr)
    {
        Util::Array<Game::Entity> stagingInstance = stagingLevel->Instantiate();
        VERIFY(stagingInstance.Size() == 1);
        VERIFY(stagingWorld.GetComponent<TestVec4>(stagingInstance[0]).v4 == Math::vec4(1, 2, 3, 4));
        stagingWorld.UnloadLevel(stagingLevel);
    }
    IO::IoServer::Instance()->DeleteFile(packedLevelPath);

    t->StopTime();
}

} // namespace Test
