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
#include "game/component/componentdata.h"

using namespace Game;
using namespace Core;

namespace Test
{
__ImplementClass(Test::CompDataTest, 'CDTS', Test::TestCase);

typedef struct 
{
    Util::String name;
	int mass;
    Math::float4 pos;
} testdata;

//------------------------------------------------------------------------------
/**
*/
void
CompDataTest::Run()
{
	Ptr<EntityManager> manager = EntityManager::Instance();
	Util::Array<Entity> entities;

	{ // Testing array of structures.
		ComponentData<testdata> component;

		Entity entity;
		uint32_t instance;
		const uint32_t n = 10000;
		// First iteration register
		for (size_t i = 0; i < n; i++)
		{
			entity = manager->NewEntity();
			entities.Append(entity);
			component.RegisterEntity(entity);
			instance = component.GetInstance(entity);
			component.data.Get<1>(instance).name = "First iteration of entities";
			component.data.Get<1>(instance).mass = i;
			component.data.Get<1>(instance).pos = Math::float4(i * 4, i * 4 + 1, i * 4 + 2, i * 4 + 3);
			if (i % 2 == 0)
			{
				component.Activate(instance);
			}
		}

		// Testing second iteration of entities inserted in old positions
		for (size_t i = 0; i < n; i++)
		{
			component.DeregisterEntity(entities[i]);
		}

		// Second iteration register
		for (size_t i = 0; i < n / 2; i++)
		{
			component.RegisterEntity(entities[i]);
			instance = component.GetInstance(entities[i]);
			component.data.Get<1>(instance).name = "Second iteration. Same entities.";
			component.data.Get<1>(instance).mass = i * 100;
			component.data.Get<1>(instance).pos = Math::float4(i * 400, i * 400 + 1, i * 400 + 2, i * 400 + 3);
			if (i % 2 == 0)
			{
				component.Activate(instance);
			}
		}

		// Third iteration unregister
		for (size_t i = 0; i < n / 2; i++)
		{
			component.DeregisterEntity(entities[i]);
		}

		// Third iteration register
		for (size_t i = 0; i < n / 2; i++)
		{
			entity = manager->NewEntity();
			entities.Append(entity);
			component.RegisterEntity(entity);
			instance = component.GetInstance(entity);
			component.data.Get<1>(instance).name = "Third iteration with new entities.";
			component.data.Get<1>(instance).mass = i * 200;
			component.data.Get<1>(instance).pos = Math::float4(i * 800, i * 800 + 1, i * 800 + 2, i * 800 + 3);
			if (i % 2 == 0)
			{
				component.Activate(instance);
			}
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
		while (i < 4)
		{
			component.Optimize();
			i++;
		}

		// Deregistering immediate on all entities
		for (size_t i = 0; i < entities.Size(); i++)
		{
			if (component.GetInstance(entities[i]) != InvalidIndex)
			{
				component.DeregisterEntityImmediate(entities[i]);
			}
		}

		// Deleting all entities
		for (SizeT i = 0; i < entities.Size(); i++)
		{
			manager->DeleteEntity(entities[i]);
		}

		// No entities should be alive at this moment, however, GC might not have been able to clean up everything yet.
		bool entityAlive = false;
		for (int i = 0; i < component.data.Size(); i++)
		{
			entityAlive = manager->IsAlive(component.data.Get<0>(i));
			if (entityAlive) break;
		}
		this->Verify(!entityAlive);

		// Clean up the rest
		component.DestroyAll();

		this->Verify(component.data.Size() == 0);
	}
	{ // Testing structure of arrays.
		entities.Clear();
		// --------------------------------------
		// 0 = Entity owner;
		// 1 = Util::String name;
		// 2 = int mass;
		// 3 = Math::float4 pos;
		ComponentData<Util::String, int, Math::float4> component;

		Entity entity;
		uint32_t instance;

		// First iteration register
		for (size_t i = 0; i < 10000; i++)
		{
			entity = manager->NewEntity();
			entities.Append(entity);
			component.RegisterEntity(entity);
			instance = component.GetInstance(entity);
			component.data.Get<1>(instance) = "First iteration of entities";
			component.data.Get<2>(instance) = i;
			component.data.Get<3>(instance) = Math::float4(i * 4, i * 4 + 1, i * 4 + 2, i * 4 + 3);
		}

		uint32_t previd = component.data.Get<0>(0).id;
		component.SetInstanceData(0, "TESTING SET", 10, Math::float4(1, 2, 3, 4));
		// Check if we really set data, but left id untouched.
		this->Verify(component.data.Get<0>(0).id == previd);
		this->Verify(component.data.Get<1>(0) == "TESTING SET");
		this->Verify(component.data.Get<2>(0) == 10);
		this->Verify(component.data.Get<3>(0) == Math::float4(1, 2, 3, 4));
		// make sure we don't set every single instance.
		this->Verify(component.data.Get<0>(1).id != previd);
		this->Verify(component.data.Get<1>(1) != "TESTING SET");
		this->Verify(component.data.Get<2>(1) != 10);
		this->Verify(component.data.Get<3>(1) != Math::float4(1, 2, 3, 4));

		// Testing second iteration of entities inserted in old positions
		for (size_t i = 0; i < 5000; i++)
		{
			component.DeregisterEntity(entities[i]);
		}

		// Second iteration register
		for (size_t i = 0; i < 5000; i++)
		{
			component.RegisterEntity(entities[i]);
			instance = component.GetInstance(entities[i]);
			component.data.Get<1>(instance) = "Second iteration. Same entities.";
			component.data.Get<2>(instance) = i * 100;
			component.data.Get<3>(instance) = Math::float4(i * 400, i * 400 + 1, i * 400 + 2, i * 400 + 3);
		}

		// Third iteration unregister
		for (size_t i = 0; i < 5000; i++)
		{
			component.DeregisterEntity(entities[i]);
		}

		// Third iteration register
		for (size_t i = 0; i < 5000; i++)
		{
			entity = manager->NewEntity();
			entities.Append(entity);
			component.RegisterEntity(entity);
			instance = component.GetInstance(entity);
			component.data.Get<1>(instance) = "Third iteration with new entities.";
			component.data.Get<2>(instance) = i * 200;
			component.data.Get<3>(instance) = Math::float4(i * 800, i * 800 + 1, i * 800 + 2, i * 800 + 3);
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
			component.Optimize();
			i++;
		}


		// Deregistering immediate on all entities
		for (size_t i = 0; i < entities.Size(); i++)
		{
			if (component.GetInstance(entities[i]) != InvalidIndex)
			{
				component.DeregisterEntityImmediate(entities[i]);
			}
		}

		// Deleting all entities
		for (SizeT i = 0; i < entities.Size(); i++)
		{
			manager->DeleteEntity(entities[i]);
		}

		// No entities should be alive at this moment, however, GC might not have been able to clean up everything yet.
		bool entityAlive = false;
		for (int i = 0; i < component.Size(); i++)
		{
			entityAlive = manager->IsAlive(component.data.Get<0>(Ids::Id32(i)));
			if (entityAlive) break;
		}
		this->Verify(!entityAlive);

		// Clean up the rest
		component.DestroyAll();

		this->Verify(component.data.Size() == 0);
	}
}
}