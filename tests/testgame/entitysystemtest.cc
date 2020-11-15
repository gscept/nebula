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

    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnBeginFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnEndFrame();

    for (int i = 0; i < 200; i++)
    {
        Game::DeleteEntity(enemies[i]);
    }

    int i;
    for (i = 0; i < 100; i++)
    {
        BaseGameFeature::BaseGameFeatureUnit::Instance()->OnBeginFrame();
        BaseGameFeature::BaseGameFeatureUnit::Instance()->OnFrame();
        BaseGameFeature::BaseGameFeatureUnit::Instance()->OnEndFrame();
        
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
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnBeginFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnEndFrame();

    Util::Queue<Game::Entity> queue;
    for (int i = 0; i < 10; i++)
    {
        Entity enemy = Game::CreateEntity({ enemyBlueprint });
        queue.Enqueue(enemy);
    }

    // Run a frame with new entities
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnBeginFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnEndFrame();

    // Delete entities from front
    for (int i = 0; i < 5; i++)
    {
        auto e = queue.Dequeue();
        Game::DeleteEntity(e);
    }

    // run a frame
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnBeginFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnEndFrame();
    
    // Delete all
    while(!queue.IsEmpty())
    {
        auto e = queue.Dequeue();
        Game::DeleteEntity(e);
    }

    // run a frame
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnBeginFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnEndFrame();

    Util::Stack<Game::Entity> stack;
    for (int i = 0; i < 10; i++)
    {
        Entity enemy = Game::CreateEntity({ enemyBlueprint });
        stack.Push(enemy);
    }

    // run a frame
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnBeginFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnEndFrame();

    // Delete entities from back
    for (int i = 0; i < 5; i++)
    {
        auto e = stack.Pop();
        Game::DeleteEntity(e);
    }

    // run a frame
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnBeginFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnEndFrame();

    // Delete all
    while (!stack.IsEmpty())
    {
        auto e = stack.Pop();
        Game::DeleteEntity(e);
    }

    // run a frame
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnBeginFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnEndFrame();

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
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnBeginFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnEndFrame();

    // Delete the last entity only
    Game::DeleteEntity(entities[9]);
    
    // run a frame
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnBeginFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnEndFrame();

    // Delete the last entity, and also another entity. This will cause
    // the entity to swap place with an invalid instance, which SHOULD be OK
    Game::DeleteEntity(entities[8]);
    Game::DeleteEntity(entities[4]);

    // run a frame
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnBeginFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnFrame();
    BaseGameFeature::BaseGameFeatureUnit::Instance()->OnEndFrame();

    VERIFY(true);

    Game::FilterSet filter({
            Game::GetPropertyId("TestHealth"_atm)
    });

	Game::Dataset set = Game::Query(filter);

	Test::TestHealth* healths = (Test::TestHealth*)set.tables[2].buffers[0];
	//Test::TestStruct* structs = (Test::TestStruct*)set.tables[0].buffers[0];

	// add a property to an entity that does not already have it. This should
	// move the entity from one category to another, effectively (in this case)
	// creating a new category, that contains only one instance (this one)
	Game::AddProperty(entities[1], Game::GetPropertyId("TestVec4"_atm));

    Msg::AddProperty::DeregisterAll();
}

}