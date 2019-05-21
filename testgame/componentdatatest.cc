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

using namespace Game;
using namespace Core;

namespace Attr
{
    __DeclareAttribute(GuidTest, Util::Guid, 'gTst', Attr::ReadWrite, Util::Guid());
    __DeclareAttribute(StringTest, Util::String, 'sTst', Attr::ReadWrite, "Default string");
    __DeclareAttribute(IntTest, int, 'iTst', Attr::ReadWrite, 1337);
    __DeclareAttribute(FloatTest, float, 'fTst', Attr::ReadWrite, 10.0f);
} // namespace Attr


namespace Test
{

__ImplementClass(Test::CompDataTest, 'CDTS', Test::TestCase);

typedef Game::Component<
    Attr::GuidTest,
    Attr::StringTest,
    Attr::IntTest,
    Attr::FloatTest
> TestComponentAllocator;

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
    component = new TestComponentAllocator({});

    Game::ComponentManager::Instance()->RegisterComponent(component, "TestComponent", 'tstc');
}

//------------------------------------------------------------------------------
/**
*/
void
TestComponent::Discard()
{
    delete component;
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
		VERIFY(component->data.Get<OWNER>(1).id != previd);
		VERIFY(component->data.Get<STRING>(1) != "TESTING SET");
		VERIFY(component->data.Get<INT>(1) != 7331);
		VERIFY(component->data.Get<FLOAT>(1) != 12345.6f);

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
		for (size_t i = 0; i < 5000; i++)
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
	}
}
}