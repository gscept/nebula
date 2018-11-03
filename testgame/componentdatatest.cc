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
	DefineGuid(GuidTest, 'gTst', Attr::ReadWrite)
	DefineStringWithDefault(StringTest, 'sTst', Attr::ReadWrite, "Default string");
	DefineIntWithDefault(IntTest, 'iTst', Attr::ReadWrite, 1337);
	DefineFloatWithDefault(FloatTest, 'fTst', Attr::ReadWrite, 10.0f);
} // namespace Attr


namespace Test
{

__ImplementClass(Test::CompDataTest, 'CDTS', Test::TestCase);
__ImplementClass(Test::TestComponent, 'tstC', Game::BaseComponent);


//------------------------------------------------------------------------------
/**
*/
TestComponent::TestComponent() :
	component_templated_t({
		Attr::GuidTest,
		Attr::StringTest,
		Attr::IntTest,
		Attr::FloatTest
	})
{
	// this->events.SetBit(ComponentEvent::OnActivate);
	// this->events.SetBit(ComponentEvent::OnDeactivate);
}

//------------------------------------------------------------------------------
/**
*/
TestComponent::~TestComponent()
{
	// Empty
}

//------------------------------------------------------------------------------
/**
*/
uint32_t
TestComponent::RegisterEntity(const Game::Entity & e)
{
	auto instance = component_templated_t::RegisterEntity(e);
	if (this->immediate_deletion)
	{
		Game::EntityManager::Instance()->RegisterDeletionCallback(e, this);
	}
	return instance;
}

//------------------------------------------------------------------------------
/**
*/
void
TestComponent::DeregisterEntity(const Game::Entity & e)
{
	if (this->immediate_deletion)
	{
		component_templated_t::DeregisterEntityImmediate(e);
		Game::EntityManager::Instance()->DeregisterDeletionCallback(e, this);
	}
	else
	{
		component_templated_t::DeregisterEntity(e);
	}
}

//------------------------------------------------------------------------------
/**
*/
SizeT
TestComponent::Optimize()
{
	if (this->immediate_deletion)
	{
		return 0;
	}
	
	return component_templated_t::Optimize();
}

template <int MEMBER> void
TestComponent::Set(const uint32_t instance, Util::Variant value)
{
	auto& v = this->data.Get<MEMBER>(instance);
	using t = decltype(v);
	v = value.Get<std::remove_reference<t>::type>();
}

//------------------------------------------------------------------------------
/**
*/
void
CompDataTest::Run()
{
	Ptr<EntityManager> manager = EntityManager::Instance();
	Util::Array<Game::Entity> entities;
	{ // Testing structure of arrays.
		entities.Clear();
		
		Ptr<TestComponent> component = TestComponent::Create();

		Game::Entity entity;
		uint32_t instance;

		// First iteration register
		for (size_t i = 0; i < 10000; i++)
		{
			entity = manager->NewEntity();
			entities.Append(entity);
			component->RegisterEntity(entity);
			instance = component->GetInstance(entity);
			component->Set<STRING>(instance, "First iteration of entities");
			component->Set<INT>(instance, 1);
			component->Set<FLOAT>(instance, float(i * 4));
		}

		// uint32_t previd = component->GetOwner(0).id;
		// component->Set(0, "TESTING SET", 10, Math::float4(1, 2, 3, 4));
		// Check if we really set data, but left id untouched.
		// VERIFY(component.data.Get<0>(0).id == previd);
		// VERIFY(component.data.Get<1>(0) == "TESTING SET");
		// VERIFY(component.data.Get<2>(0) == 10);
		// VERIFY(component.data.Get<3>(0) == Math::float4(1, 2, 3, 4));
		// make sure we don't set every single instance.
		// VERIFY(component.data.Get<0>(1).id != previd);
		// VERIFY(component.data.Get<1>(1) != "TESTING SET");
		// VERIFY(component.data.Get<2>(1) != 10);
		// VERIFY(component.data.Get<3>(1) != Math::float4(1, 2, 3, 4));

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
			// component.data.Get<1>(instance) = "Second iteration. Same entities.";
			// component.data.Get<2>(instance) = i * 100;
			// component.data.Get<3>(instance) = Math::float4(i * 400, i * 400 + 1, i * 400 + 2, i * 400 + 3);
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
			// component.Get<1>(instance) = "Third iteration with new entities.";
			// component.Get<2>(instance) = i * 200;
			// component.Get<3>(instance) = Math::float4(i * 800, i * 800 + 1, i * 800 + 2, i * 800 + 3);
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