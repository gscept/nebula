//------------------------------------------------------------------------------
//  componenttest.cc
//  (C) 2017 Individual contributors, see AUTHORS file
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
	Entity owner;
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

	ComponentData<testdata> component;

	Entity entity;
	uint32_t instance;

	// First iteration register
	for (size_t i = 0; i < 10000; i++)
	{
		entity = manager->NewEntity();
		entities.Append(entity);
		component.RegisterEntity(entity);
		instance = component.GetInstance(entity);
		component.data[instance].name = "First iteration of entities";
		component.data[instance].mass = i;
		component.data[instance].pos = Math::float4(i * 4, i * 4 + 1, i * 4 + 2, i * 4 + 3);
	}

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
		component.data[instance].name = "Second iteration. Same entities.";
		component.data[instance].mass = i * 100;
		component.data[instance].pos = Math::float4(i * 400, i * 400 + 1, i * 400 + 2, i * 400 + 3);
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
		component.data[instance].name = "Third iteration with new entities.";
		component.data[instance].mass = i * 200;
		component.data[instance].pos = Math::float4(i * 800, i * 800 + 1, i * 800 + 2, i * 800 + 3);
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
	for (int i = 0; i < component.data.Size(); i++)
	{
		entityAlive = manager->IsAlive(component.data[i].owner);
		if (entityAlive) break;
	}
	this->Verify(!entityAlive);

	// Clean up the rest
	component.DestroyAll();

	this->Verify(component.data.IsEmpty());
}
}