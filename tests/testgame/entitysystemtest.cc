//------------------------------------------------------------------------------
//  entitysystemtest.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "entitysystemtest.h"
#include "timing/timer.h"
#include "basegamefeature/managers/entitymanager.h"
#include "basegamefeature/managers/blueprintmanager.h"
#include "basegamefeature/basegamefeatureunit.h"
#include "testproperties.h"
#include "basegamefeature/messages/entitymessages.h"

using namespace Game;
using namespace Math;
using namespace Util;

namespace Test
{

static int numFramesExecuted = 0;

//------------------------------------------------------------------------------

__ImplementClass(Test::EntitySystemTest, 'GEST', Test::TestCase);


void StepFrame()
{
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnBeginFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnEndFrame();
}

struct ManagedTestProperty
{
    bool destroyed = false;
};

//------------------------------------------------------------------------------
/**
    @todo   this test should be more thorough and make sure that instances retain
            the correct data after shuffeling around (deletion, creation, defrag etc.)
*/
void
EntitySystemTest::Run()
{
    BlueprintId const playerBlueprint = Game::GetBlueprintId("Player"_atm);
    BlueprintId const enemyBlueprint = Game::GetBlueprintId("Enemy"_atm);

    for (int i = 0; i < 500; i++)
    {
        Entity player = Game::CreateEntity({ playerBlueprint });
    }

    Util::Array<Entity> enemies;
    for (int i = 0; i < 500; i++)
    {
        Entity enemy = Game::CreateEntity({ enemyBlueprint });
        enemies.Append(enemy);
    }

    StepFrame();

    for (int i = 0; i < 200; i++)
    {
        Game::DeleteEntity(enemies[i]);
    }

    int i;
    for (i = 0; i < 100; i++)
    {
        StepFrame();
        
        if (i % 5 == 1)
        {
            Game::DeleteEntity(enemies[i + 200]);
        }
    }

    // Delete all entities
    for (auto e : enemies)
    {
        if (Game::IsValid(e))
            Game::DeleteEntity(e);
    }

    // Run a frame
    StepFrame();

    Util::Queue<Game::Entity> queue;
    for (int i = 0; i < 10; i++)
    {
        Entity enemy = Game::CreateEntity({ enemyBlueprint });
        queue.Enqueue(enemy);
    }

    // Run a frame with new entities
    StepFrame();

    // Delete entities from front
    for (int i = 0; i < 5; i++)
    {
        auto e = queue.Dequeue();
        Game::DeleteEntity(e);
    }

    // run a frame
    StepFrame();
    
    // Delete all
    while(!queue.IsEmpty())
    {
        auto e = queue.Dequeue();
        Game::DeleteEntity(e);
    }

    // run a frame
    StepFrame();

    Util::Stack<Game::Entity> stack;
    for (int i = 0; i < 10; i++)
    {
        Entity enemy = Game::CreateEntity({ enemyBlueprint });
        stack.Push(enemy);
    }

    // run a frame
    StepFrame();

    // Delete entities from back
    for (int i = 0; i < 5; i++)
    {
        auto e = stack.Pop();
        Game::DeleteEntity(e);
    }

    // run a frame
    StepFrame();

    // Delete all
    while (!stack.IsEmpty())
    {
        auto e = stack.Pop();
        Game::DeleteEntity(e);
    }

    // run a frame
    StepFrame();

    Game::Entity entities[10] =
    {
        Game::CreateEntity({ playerBlueprint }),
        Game::CreateEntity({ playerBlueprint }),
        Game::CreateEntity({ playerBlueprint }),
        Game::CreateEntity({ playerBlueprint }),
        Game::CreateEntity({ playerBlueprint }),
        Game::CreateEntity({ playerBlueprint }),
        Game::CreateEntity({ playerBlueprint }),
        Game::CreateEntity({ playerBlueprint }),
        Game::CreateEntity({ playerBlueprint }),
        Game::CreateEntity({ playerBlueprint })
    };

    // run a frame
    StepFrame();

    // Delete the last entity only
    Game::DeleteEntity(entities[9]);
    
    // run a frame
    StepFrame();

    // Delete the last entity, and also another entity. This will cause
    // the entity to swap place with an invalid instance, which SHOULD be OK
    Game::DeleteEntity(entities[8]);
    Game::DeleteEntity(entities[4]);

    // run a frame
    StepFrame();

    VERIFY(true);

    // Test managed properties
    {
        PropertyId managedPropertyDescriptor;
        {
            ManagedTestProperty defVal;
            Game::PropertyCreateInfo propertyInfo;
            propertyInfo.name = "ManagedTestProperty";
            propertyInfo.byteSize = sizeof(ManagedTestProperty);
            propertyInfo.defaultValue = &defVal;
            propertyInfo.flags = PropertyFlags::PROPERTYFLAG_MANAGED;
            managedPropertyDescriptor = Game::CreateProperty(propertyInfo);
        }

        Game::Entity enemies[] = {
            Game::CreateEntity({ enemyBlueprint, TemplateId::Invalid(), true }),
            Game::CreateEntity({ enemyBlueprint, TemplateId::Invalid(), true }),
            Game::CreateEntity({ enemyBlueprint, TemplateId::Invalid(), true }),
            Game::CreateEntity({ enemyBlueprint, TemplateId::Invalid(), true }),
            Game::CreateEntity({ enemyBlueprint, TemplateId::Invalid(), true })
        };

        {
            Category const& cat = EntityManager::Instance()->GetCategory(Game::GetEntityMapping(enemies[0]).category);
            VERIFY(!Game::GetWorldDatabase()->IsValid(cat.managedPropertyTable));
            VERIFY(Game::GetWorldDatabase()->GetNumRows(cat.instanceTable) == 5);
        }

        Game::Op::RegisterProperty regOp;
        regOp.entity = enemies[0];
        regOp.pid = managedPropertyDescriptor;
        Game::Execute(regOp);
        regOp.entity = enemies[1];
        regOp.pid = managedPropertyDescriptor;
        Game::Execute(regOp);
        regOp.entity = enemies[2];
        regOp.pid = managedPropertyDescriptor;
        Game::Execute(regOp);

        {
            Category const& cat = EntityManager::Instance()->GetCategory(Game::GetEntityMapping(enemies[0]).category);
            VERIFY(Game::GetWorldDatabase()->IsValid(cat.managedPropertyTable));
            VERIFY(Game::GetWorldDatabase()->GetNumRows(cat.instanceTable) == 3);
        }

        StepFrame();

        // delete an entity
        Game::DeleteEntity(enemies[2]);

        StepFrame();

        {
            Category const& cat = EntityManager::Instance()->GetCategory(Game::GetEntityMapping(enemies[0]).category);
            VERIFY(Game::GetWorldDatabase()->GetNumRows(cat.managedPropertyTable) == 1);
            VERIFY(Game::GetWorldDatabase()->GetNumRows(cat.instanceTable) == 2);
        }

        StepFrame();

        {
            Category const& cat = EntityManager::Instance()->GetCategory(Game::GetEntityMapping(enemies[0]).category);
            VERIFY(Game::GetWorldDatabase()->GetNumRows(cat.managedPropertyTable) == 0);
            VERIFY(Game::GetWorldDatabase()->GetNumRows(cat.instanceTable) == 2);
        }
    }

    Game::FilterCreateInfo filterInfo;
    filterInfo.inclusive[0] = Game::GetPropertyId("TestHealth");
    filterInfo.access[0] = Game::AccessMode::READ;
    filterInfo.inclusive[1] = Game::GetPropertyId("TestStruct");
    filterInfo.access[1] = Game::AccessMode::WRITE;
    filterInfo.numInclusive = 2;
    Game::Filter filter = Game::CreateFilter(filterInfo);

    Game::Dataset set = Game::Query(filter);
    
    VERIFY(set.numViews == 3);

    Test::TestHealth* healths = (Test::TestHealth*)set.views[0].buffers[0];
    Test::TestStruct* structs = (Test::TestStruct*)set.views[0].buffers[1];

    // add a property to an entity that does not already have it. This should
    // move the entity from one category to another, effectively (in this case)
    // creating a new category, that contains only one instance (this one)
    Game::Op::RegisterProperty registerOp;
    registerOp.entity = entities[1];
    registerOp.pid = Game::GetPropertyId("TestVec4"_atm);
    registerOp.value = nullptr;
    Game::Execute(registerOp);
}

}