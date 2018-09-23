//------------------------------------------------------------------------------
//  levelloader.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "levelloader.h"
#include "util/hashtable.h"
#include "scenecompiler.h"
#include "basegamefeature/managers/entitymanager.h"
#include "basegamefeature/managers/componentmanager.h"

#include <chrono>
#include <ctime>

namespace BaseGameFeature
{

//void RegisterNodes(int parent, Node node, Scene& scene, HashTable<Entity, uint>& entities)
//{
//	// Add entity to index map entry
//	entities.Add(node.entity, scene.numEntities);
//	scene.numEntities++;
//	scene.parentIndices.Append(parent)
//	for (auto child : node.children)
//		RegisterNodes(scene.numEntities - 1, child, scene);
//}


//------------------------------------------------------------------------------
/**
*/
bool
LevelLoader::Save(const Util::String& levelName)
{
	auto tstart = std::chrono::system_clock::now();

	auto numEntities = Game::EntityManager::Instance()->GetNumEntities();
	SceneCompiler scene;
	uint hashedEntities = 0;
	Util::HashTable<Game::Entity, uint> entities;

	scene.numEntities = numEntities;

	// Fill components

	Ptr<Game::ComponentManager> manager = Game::ComponentManager::Instance();
	scene.numComponents = manager->GetNumComponents();
	for (SizeT i = 0; i < scene.numComponents; i++)
	{
		Ptr<Game::BaseComponent> component = manager->GetComponentAtIndex(i);
		SceneComponent c;

		c.fourcc = component->GetClassFourCC();
		c.numInstances = component->GetNumInstances();

		// Update update each entity attribute
		auto entityAttributes = component->GetEntityAttributes();
		for (SizeT k = 0; k < entityAttributes.Size(); k++)
		{
			for (SizeT j = 0; j < entityAttributes[k]->Size(); j++)
			{
				// Update all entity ids to the indices they're located at.
				// When loading we can then switch back to real entity ids.
				if (entities.Contains((*entityAttributes[k])[j]))
				{
					(*entityAttributes[k])[j] = entities[(*entityAttributes[k])[j]];
				}
				else
				{
					// hash entity
					entities.Add((*entityAttributes[k])[j], hashedEntities);
					(*entityAttributes[k])[j] = hashedEntities;
					hashedEntities++;
				}
			}
		}

		c.data = component->GetBlob();

		scene.components.Append(c);
	}
	scene.Compile(levelName);
	
	auto tend = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = tend - tstart;
	n_printf("Save time: %f\n", elapsed_seconds.count());

	return false;
}


//------------------------------------------------------------------------------
/**
*/
bool
LevelLoader::Load(const Util::String& levelName)
{
	auto tstart = std::chrono::system_clock::now();
	SceneCompiler scene;

	scene.Decompile(levelName);
	
	Util::Array<Game::Entity> entities = Game::EntityManager::Instance()->CreateEntities(scene.numEntities);
		
	for (auto component : scene.components)
	{
		Ptr<Game::BaseComponent> c = Game::ComponentManager::Instance()->ComponentByFourCC(component.fourcc);
		if (c.isvalid())
		{
			// Needs to create entirely new instances, not reuse old.

			uint start = c->GetNumInstances();
			c->AllocInstances(component.numInstances);
			uint end = c->GetNumInstances();

			c->SetBlob(component.data, start, component.numInstances);
			auto entityAttributes = c->GetEntityAttributes();
			for (SizeT k = 0; k < entityAttributes.Size(); k++)
			{
				for (uint j = start; j < end; j++)
				{
					// Update all entity ids to the newly generated.
					(*entityAttributes[k])[j] = entities[(*entityAttributes[k])[j].id];
				}
			}

			c->SetParents(start, end, entities, scene.parentIndices);
		}
	}

	auto tend = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = tend - tstart;
	n_printf("Load time: %f\n", elapsed_seconds.count());

	return true;
}

} // namespace BaseGameFeature
