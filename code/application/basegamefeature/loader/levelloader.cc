//------------------------------------------------------------------------------
//  levelloader.cc
//  (C) 2018-2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "levelloader.h"
#include "util/hashtable.h"
#include "scenecompiler.h"
#include "basegamefeature/managers/entitymanager.h"
#include "io/memorystream.h"
#include "basegamefeature/managers/componentmanager.h"
#include "basegamefeature/components/transformcomponent.h"
#include "io/filestream.h"

// Generated Flatbuffer header
#include "level.h"

namespace BaseGameFeature
{

//------------------------------------------------------------------------------
/**
*/
bool
LevelLoader::Save(const Util::String& levelName)
{
    flatbuffers::FlatBufferBuilder builder(1024);
    Game::Serialization::LevelBuilder sceneBuilder(builder);

    Game::Serialization::EntityBundleBuilder entitiesBuilder(builder);
    
    auto numEntities = Game::EntityManager::Instance()->GetNumEntities();
	// SceneCompiler scene;
	uint indexHash = 0;
	Util::HashTable<Game::Entity, uint, 1024> entityToIndex;

	// scene.numEntities = numEntities;
    entitiesBuilder.add_numEntities(numEntities);

	// Fill components

	Ptr<Game::ComponentManager> manager = Game::ComponentManager::Instance();
    uint numComponents = 0;
	uint numGameComponents = manager->GetNumComponents();

    Util::Array<flatbuffers::Offset<Game::Serialization::ComponentBuildData>> componentsData;

	for (SizeT i = 0; i < numGameComponents; i++)
	{
		Game::ComponentInterface* component = manager->GetComponentAtIndex(i);
		if (component->NumRegistered() == 0)
		{
			// Skip this component
			continue;
		}

		// We need to clean up and optimize any potential garbage.
		component->Clean();

		// this component is part of scene.
		numComponents++;

		// ComponentBuildData c;
        Game::Serialization::ComponentBuildDataBuilder c(builder);

        auto numInstances = component->NumRegistered();
		c.add_fourcc(component->GetIdentifier().AsUInt());
		c.add_numInstances(numInstances);

		// TODO: Add description of component so that we can make sure we're not reading incorrect or outdated data later.
		// ex. c.description = Util::String(for each attribute: { return attributeDefinition.ToString() })

		// Serialize the component data into builddata local stream
		Ptr<IO::BinaryWriter> bWriter = IO::BinaryWriter::Create();
        Ptr<IO::MemoryStream> mStream = IO::MemoryStream::Create();
		bWriter->SetStream(mStream);
		bWriter->Open();
		component->SerializeOwners(bWriter);

		if (component->functions.Serialize != nullptr)
			component->functions.Serialize(bWriter);

		bWriter->Close();
		
		// We know the owner is always the first attribute
		// so we can easily update owner of each instance
		{
			Game::Entity* owners = (Game::Entity*)mStream->GetRawPointer();
			
			// update owner to the entity index
            // when the buffer is loaded later on, the owners will be in the 0...numInstances range
            // this means that later on when we load the data, we need to convert the owner ids to the newly allocated entity ids.
			for (uint instance = 0; instance < numInstances; instance++)
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

        // Add the components data stream to the builder
        c.add_dataStream(builder.CreateVector((ubyte*)mStream->GetRawPointer(), mStream->GetSize()));

        componentsData.Append(c.Finish());
	}
    entitiesBuilder.add_components(builder.CreateVector(componentsData.Begin(), componentsData.Size()));

    entitiesBuilder.add_numComponents(numComponents);

    Util::Array<uint> parentIndices;
    // Create the scene hierarchy parent indices list
    // These will be modified to the correct instance ids (relative to the newly allocated instances ids) upon load.
	{
		SizeT numRegisteredTransforms = Game::TransformComponent::NumRegistered();
		for (int i = 0; i < numRegisteredTransforms; ++i)
		{
			uint32_t parentInstance = Game::TransformComponent::GetParent(i);
			if (parentInstance == InvalidIndex)
			{
				parentIndices.Append(-1);
			}
			else
			{
				Game::Entity parentEntity = Game::TransformComponent::GetOwner(parentInstance);
				parentIndices.Append(entityToIndex[parentEntity]);
			}
		}
	}
    entitiesBuilder.add_parentIndices(builder.CreateVector(parentIndices.Begin(), parentIndices.Size()));

    sceneBuilder.add_entities(entitiesBuilder.Finish());

    sceneBuilder.Finish();

    auto uri = IO::URI(levelName);
    Ptr<IO::BinaryWriter> fileWriter = IO::BinaryWriter::Create();
    Ptr<IO::FileStream> fileStream = IO::FileStream::Create();
    fileStream->SetAccessMode(IO::Stream::AccessMode::WriteAccess);
    fileStream->SetURI(uri);
    fileStream->Open();
    fileWriter->SetStream(fileStream);
	
    fileWriter->WriteRawData(builder.GetBufferPointer(), builder.GetSize());

    fileStream->Close();

	return true;
}

struct Listener
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
	SceneCompiler scene;

    if (!scene.Decompile(levelName))
    {
        return false;
    }
	
	Util::Array<Game::Entity> entities = Game::EntityManager::Instance()->CreateEntities(scene.numEntities);
		
	// We need to save each component and enitity start index so that we can call activate after
	// all components has been loaded
	Util::Array<Listener> activateListeners;

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
			
			c->DeserializeOwners(bReader, start, component.numInstances);
			if (c->functions.Deserialize != nullptr)
				c->functions.Deserialize(bReader, start, component.numInstances);
			
			bReader->Close();

			// update owner and id maps
			for (uint j = start; j < end; j++)
			{
				c->SetOwner(j, entities[c->GetOwner(j).id]);
			}

			if (c->functions.SetParents != nullptr && scene.parentIndices.Size() > 0)
			{
				c->functions.SetParents(start, end, entities, scene.parentIndices);
			}

			if (c->SubscribedEvents().IsSet(Game::ComponentEvent::OnLoad) && c->functions.OnLoad != nullptr)
			{
				SizeT end = start + component.numInstances;
				for (SizeT i = start; i < end; i++)
				{
					c->functions.OnLoad(i);
				}
			}

			if (c->SubscribedEvents().IsSet(Game::ComponentEvent::OnActivate) && c->functions.OnActivate != nullptr)
			{
				// Add to list to that we can activate all instances in this component later.
				Listener listener;
				listener.component = c;
				listener.firstInstance = start;
				listener.numInstances = component.numInstances;
				activateListeners.Append(listener);
			}
		}
	}

	for (auto listener : activateListeners)
	{
		SizeT end = listener.firstInstance + listener.numInstances;
		for (SizeT i = listener.firstInstance; i < end; i++)
		{
			listener.component->functions.OnActivate(i);
		}
	}

	return true;
}

} // namespace BaseGameFeature
