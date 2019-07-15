//------------------------------------------------------------------------------
//  componenttest.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "componentdatatest.h"
#include "foundation.h"
#include "core/coreserver.h"
#include "core/sysfunc.h"
#include "testbase/testrunner.h"
#include "basegamefeature/managers/entitymanager.h"
#include "basegamefeature/managers/componentmanager.h"
#include "game/component/component.h"

using namespace Game;
using namespace Core;

namespace Test
{

__ImplementClass(Test::CompDataTest, 'CDTS', Test::TestCase);

static TestComponentAllocator* component;

enum AttributeNames
{
	OWNER,
	GUID,
	STRING,
	INT,
	FLOAT,
	NumAttributes
};

__ImplementComponent(TestComponent, component);

//------------------------------------------------------------------------------
/**
*/
void
TestComponent::Create()
{
	component = new TestComponentAllocator();
	Game::ComponentManager::Instance()->RegisterComponent(component, "TestComponent", 'TSTC');

}

//------------------------------------------------------------------------------
/**
*/
void
TestComponent::Discard()
{
	Game::ComponentManager::Instance()->DeregisterComponent(component);
	// delete later, since we want to verify that deregister works properly first.
}

//------------------------------------------------------------------------------
/**
*/
void
CompDataTest::Run()
{
	Ptr<EntityManager> manager = EntityManager::Instance();
	
	TestComponent::Create();
	
	Util::Array<Game::Entity> entities;
	{ // Testing structure of arrays.
		entities.Clear();
		
		Game::Entity entity;
		uint32_t instance;

		// First iteration register
		for (size_t i = 0; i < 10000; i++)
		{
			entity = manager->NewEntity();
			entities.Append(entity);
			component->RegisterEntity(entity);
			instance = component->GetInstance(entity);
			component->data.Get<STRING>(instance) = "First iteration of entities";
			component->data.Get<INT>(instance) = 1;
			component->data.Get<FLOAT>(instance) = float(i * 4);
		}

		uint32_t previd = component->GetOwner(0).id;
		component->SetInstanceData(0, Util::Guid(), "TESTING SET", 7331, 12345.6f);
		// Check if we really set data, but left id untouched.
		VERIFY(component->data.Get<OWNER>(0).id == previd);
		VERIFY(component->data.Get<STRING>(0) == "TESTING SET");
		VERIFY(component->data.Get<INT>(0) == 7331);
		VERIFY(component->data.Get<FLOAT>(0) == 12345.6f);
		// make sure we don't set every single instance.
		VERIFY(component->Get<Attr::Owner>(1).id != previd);
		VERIFY(component->Get<Attr::StringTest>(1) != "TESTING SET");
		VERIFY(component->Get<Attr::IntTest>(1) != 7331);
		VERIFY(component->Get<Attr::FloatTest>(1) != 12345.6f);

		// Testing second iteration of entities inserted in old positions
		for (size_t i = 0; i < 5000; i++)
		{
			component->DeregisterEntity(entities[i]);
		}

		// Second iteration register
		for (size_t i = 0; i < 5000; i++)
		{
			component->RegisterEntity(entities[i]);
			instance = component->GetInstance(entities[i]);
			component->data.Get<STRING>(instance) = "Second iteration. Same entities.";
			component->data.Get<INT>(instance) = i * 100;
			component->data.Get<FLOAT>(instance) = float(i * 400);
		}

		// Third iteration unregister
		for (size_t i = 0; i < 5000; i++)
		{
			component->DeregisterEntity(entities[i]);
		}

		// Third iteration register
		for (size_t i = 5000; i < 10000; i++)
		{
			entity = manager->NewEntity();
			entities.Append(entity);
			component->RegisterEntity(entity);
			instance = component->GetInstance(entity);
			component->data.Get<STRING>(instance) = "Third iteration with new entities.";
			component->data.Get<INT>(instance) = i * 200;
			component->data.Get<FLOAT>(instance) = float(i * 800);
		}

		// Testing optimization
		// Testing garbage collection
		SizeT numToDelete = entities.Size() / 4;
		for (SizeT i = 0; i < numToDelete; i++)
		{
			SizeT index = entities.Size() - 1;
			manager->DeleteEntity(entities[index]);
			entities.EraseIndex(index);
		}

		// optimization of dataset
		int i = 0;
		while (i < 10000)
		{
			component->Optimize();
			i++;
		}


		// Deregistering immediate on all entities
		for (size_t i = 0; i < entities.Size(); i++)
		{
			if (component->GetInstance(entities[i]) != InvalidIndex)
			{
				component->DeregisterEntityImmediate(entities[i]);
			}
		}

		// Deleting all entities
		for (SizeT i = 0; i < entities.Size(); i++)
		{
			manager->DeleteEntity(entities[i]);
		}

		// No entities should be alive at this moment, however, GC might not have been able to clean up everything yet.
		bool entityAlive = false;
		for (int i = 0; i < component->NumRegistered(); i++)
		{
			entityAlive = manager->IsAlive(component->GetOwner(i));
			if (entityAlive) break;
		}
		VERIFY(!entityAlive);

		// Clean up the rest
		component->DestroyAll();

		VERIFY(component->NumRegistered() == 0);

        // Make sure that the order of deregistrations does not matter
        entities.Clear();
        for (size_t i = 0; i < 5; i++)
        {
            entities.Append(EntityManager::Instance()->NewEntity());
            component->RegisterEntity(entities[i]);
        }
        
        component->DeregisterEntity(entities[0]);
        component->DeregisterEntity(entities[2]);
        component->DeregisterEntity(entities[4]);

        // Clear freeids list
        // Might crash here if we don't handle it correctly
        component->Optimize();

        component->DeregisterEntity(entities[3]);
        component->Optimize();
        component->DeregisterEntity(entities[1]);
        component->Optimize();

        // Passed crashtest!
        VERIFY(true);

        component->Clean();
        component->DeregisterAllInactive();
        component->DestroyAll();

		TestComponent::Discard();

		VERIFY(Game::ComponentManager::Instance()->GetComponentByFourCC('TSTC') == nullptr);
		VERIFY(Game::ComponentManager::Instance()->GetComponentByName("TestComponent") == nullptr);

		delete component;
	}
}
}
