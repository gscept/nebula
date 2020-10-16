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

using namespace Game;
using namespace Math;
using namespace Util;

namespace Test
{

static int numFramesExecuted = 0;

//------------------------------------------------------------------------------
/**
*/
//void
//TestProperty::OnBeginFrame()
//{
//    const SizeT num = Game::GetNumInstances(this->category);
//    for (IndexT i = 0; i < num; i++)
//    {
//        this->data.health[i] += 1;
//        
//        Math::vec4 const& move = this->data.moveDirection[i];
//        Math::vec4 newPos = this->data.transform[i].position;
//        newPos += this->data.moveDirection[i] * this->data.speed[i];
//        this->data.transform[i].position = newPos;
//    }
//    numFramesExecuted++;
//}

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
    CategoryCreateInfo info;
    info.name = "Enemy";
	info.columns = {
		Game::GetPropertyId("WorldTransform"_atm),
		Game::GetPropertyId("TestStruct"_atm),
		Game::GetPropertyId("TestVec4"_atm),
		Game::GetPropertyId("TestHealth"_atm)
    };
    EntityManager::Instance()->AddCategory(info);

    info.name = "Player";
    info.columns.Clear();
	info.columns = {
		Game::GetPropertyId("TestHealth"_atm),
		Game::GetPropertyId("TestStruct"_atm),
		Game::GetPropertyId("WorldTransform"_atm)
	};

    EntityManager::Instance()->AddCategory(info);

    CategoryId const playerCategory = Game::GetCategoryId("Player"_atm);
    CategoryId const enemyCategory = Game::GetCategoryId("Enemy"_atm);

    for (int i = 0; i < 500; i++)
    {
        Entity player = Game::CreateEntity({ playerCategory });
    }

    Util::Array<Entity> enemies;
    for (int i = 0; i < 500; i++)
    {
        Entity enemy = Game::CreateEntity({ enemyCategory });
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
        Entity enemy = Game::CreateEntity({ enemyCategory });
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
        Entity enemy = Game::CreateEntity({ enemyCategory });
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
        Game::CreateEntity({ enemyCategory }),
        Game::CreateEntity({ enemyCategory }),
        Game::CreateEntity({ enemyCategory }),
        Game::CreateEntity({ enemyCategory }),
        Game::CreateEntity({ enemyCategory }),
        Game::CreateEntity({ enemyCategory }),
        Game::CreateEntity({ enemyCategory }),
        Game::CreateEntity({ enemyCategory }),
        Game::CreateEntity({ enemyCategory }),
        Game::CreateEntity({ enemyCategory })
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

    // Test query and filtering
	/*
    Game::FilterSet filter;
    filter.inclusive = {
        Attr::Runtime::HealthId,
        Attr::Runtime::SpeedId
    };

    Game::Dataset set = Game::Query(filter);

    auto const enemyCat = Game::GetCategoryId("Enemy"_atm);
    auto const playerCat = Game::GetCategoryId("Player"_atm);
    auto const otherCat = Game::GetCategoryId("Other"_atm);

    VERIFY(set.categories.Size() == 2);
    //VERIFY(set.categories.FindIndex(enemyCat) != InvalidIndex);
    //VERIFY(set.categories.FindIndex(playerCat) != InvalidIndex);
    //VERIFY(set.categories.FindIndex(otherCat) == InvalidIndex);

    filter.inclusive = {
        Attr::Runtime::HealthId,
        Attr::Runtime::TargetId
    };

    set = Game::Query(filter);
    
    VERIFY(set.categories.Size() == 1);
    //VERIFY(set.categories.FindIndex(enemyCat) != InvalidIndex);
    //VERIFY(set.categories.FindIndex(playerCat) == InvalidIndex);
    //VERIFY(set.categories.FindIndex(otherCat) == InvalidIndex);

    filter.inclusive = {
        Attr::Runtime::HealthId
    };

    filter.exclusive = {
        Attr::Runtime::TargetId
    };

    set = Game::Query(filter);

    VERIFY(set.categories.Size() == 1);
    //VERIFY(set.categories.FindIndex(playerCat) != InvalidIndex);
    //VERIFY(set.categories.FindIndex(enemyCat) == InvalidIndex);
    //VERIFY(set.categories.FindIndex(otherCat) == InvalidIndex);
	*/

    TestMsg::Send(10, 20.0f);

    auto msgQueue = TestMsg::AllocateMessageQueue();
    TestMsg::Defer(msgQueue, 100, 200.0f);
    TestMsg::DispatchMessageQueue(msgQueue);
}



}