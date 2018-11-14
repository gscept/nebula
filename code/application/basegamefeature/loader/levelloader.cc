//------------------------------------------------------------------------------
//  levelloader.cc
//  (C) 2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "levelloader.h"
#include "util/hashtable.h"
#include "scenecompiler.h"
#include "basegamefeature/managers/entitymanager.h"
#include "io/memorystream.h"
#include "basegamefeature/managers/componentmanager.h"

#include <chrono>
#include <ctime>

namespace BaseGameFeature
{

//------------------------------------------------------------------------------
/**
*/
bool
LevelLoader::Save(const Util::String& levelName)
{
	auto tstart = std::chrono::system_clock::now();

	auto numEntities = Game::EntityManager::Instance()->GetNumEntities();
	SceneCompiler scene;
	uint indexHash = 0;
	Util::HashTable<Game::Entity, uint> entityToIndex(numEntities);

	scene.numEntities = numEntities;

	// Fill components

	Ptr<Game::ComponentManager> manager = Game::ComponentManager::Instance();
	scene.numComponents = 0;
	uint numComponents = manager->GetNumComponents();
	for (SizeT i = 0; i < numComponents; i++)
	{
		Game::ComponentInterface* component = manager->GetComponentAtIndex(i);
		if (component->NumRegistered() == 0)
		{
			// Skip this component
			continue;
		}
		// this component is part of scene.
		scene.numComponents++;

		ComponentBuildData c;

		c.fourcc = component->GetRtti()->GetFourCC();
		c.numInstances = component->NumRegistered();

		// TODO: Add description of component so that we can make sure we're not reading incorrect or outdated data later.
		// ex. c.description = Util::String(for each attribute: { return attributeDefinition.ToString() })

		// Serialize the component data into builddata local stream
		Ptr<IO::BinaryWriter> bWriter = IO::BinaryWriter::Create();

		c.InitializeStream();
		bWriter->SetStream(c.mStream);
		bWriter->Open();
		component->functions.Serialize(bWriter);
		bWriter->Close();

		// We know the owner is always the first attribute
		// so we can easily update owner of each instance
		{
			Game::Entity* owners = (Game::Entity*)c.mStream->GetRawPointer();
			
			// update owner to the entity index
			for (uint instance = 0; instance < c.numInstances; instance++)
			{
				Game::Entity owner = owners[instance];
				if (entityToIndex.Contains(owner))
				{
					owners[instance] = entityToIndex[owner];
				}
				else
				{
					// hash entity
					entityToIndex.Add(owner, indexHash);
					owners[instance] = indexHash;
					indexHash++;
				}
			}
		}

		scene.components.Append(c);
	}
	scene.Compile(levelName);
	
	auto tend = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = tend - tstart;
	n_printf("Save time: %f\n", elapsed_seconds.count());

	return true;
}

struct ActivateListener
{
	Game::ComponentInterface* component;
	uint firstInstance;
	uint numInstances;
};

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
		
	// We need to save each component and enitity start index so that we can call activate after
	// all components has been loaded
	Util::Array<ActivateListener> activateListeners;

	for (auto component : scene.components)
	{
		Game::ComponentInterface* c = Game::ComponentManager::Instance()->GetComponentByFourCC(component.fourcc);
		if (c != nullptr)
		{
			// Needs to create entirely new instances, not reuse old.

			uint start = c->NumRegistered();
			c->Allocate(component.numInstances);
			uint end = c->NumRegistered();

			Ptr<IO::BinaryReader> bReader = IO::BinaryReader::Create();
			bReader->SetStream(component.mStream);
			bReader->Open();
			
			c->functions.Deserialize(bReader, start, component.numInstances);
			
			bReader->Close();

			// update owner and id maps
			for (uint j = start; j < end; j++)
			{
				c->SetOwner(j, entities[c->GetOwner(j).id]);
			}

			if (c->functions.SetParents != nullptr)
				c->functions.SetParents(start, end, entities, scene.parentIndices);

			if (c->SubscribedEvents().IsSet(Game::ComponentEvent::OnActivate) && c->functions.OnActivate != nullptr)
			{
				// Add to list to that we can activate all instances in this component later.
				ActivateListener listener;
				listener.component = c;
				listener.firstInstance = start;
				listener.numInstances = component.numInstances;
				activateListeners.Append(listener);
			}
		}
	}


	for (auto listener : activateListeners)
	{
		for (SizeT i = listener.firstInstance; i < listener.numInstances; i++)
		{
			listener.component->functions.OnActivate(i);
		}
	}

	auto tend = std::chrono::system_clock::now();
	std::chrono::duration<double> elapsed_seconds = tend - tstart;
	n_printf("Load time: %f\n", elapsed_seconds.count());

	return true;
}

} // namespace BaseGameFeature
