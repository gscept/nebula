//------------------------------------------------------------------------------
//  levelloader.cc
//  (C) 2018-2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "levelloader.h"
#include "util/hashtable.h"
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
    
    auto numEntities = Game::EntityManager::Instance()->GetNumEntities();
	uint indexHash = 0;
	Util::HashTable<Game::Entity, uint, 1024> entityToIndex;

	Ptr<Game::ComponentManager> manager = Game::ComponentManager::Instance();
    uint numComponents = 0;
	uint numGameComponents = manager->GetNumComponents();

    Util::Array<flatbuffers::Offset<Game::Serialization::ComponentData>> componentsData;

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

		auto numInstances = component->NumRegistered();
        
        // Add description of component so that we can make sure we're not reading incorrect or outdated data later.
		Util::Array<flatbuffers::Offset<Game::Serialization::AttributeDefinition>> attributeDefinitions;
        auto const& attrs = component->GetAttributes();

        for (auto attr : attrs)
        {
            attributeDefinitions.Append(
                Game::Serialization::CreateAttributeDefinitionDirect(
                    builder,
                    attr.fourcc.AsUInt(),
                    attr.name.AsCharPtr(),
                    attr.typeName.AsCharPtr()
                )
            );
        }

        auto attributes = builder.CreateVector(attributeDefinitions.Begin(), attributeDefinitions.Size());

        Util::Array<Game::Entity> instanceOwners = component->GetOwners();

        // update owner to the entity index
        // when the buffer is loaded later on, the owners will be in the 0...numInstances range
        // this means that later on when we load the data, we need to convert the owner ids to the newly allocated entity ids.
        for (uint instance = 0; instance < numInstances; instance++)
        {
            Game::Entity owner = instanceOwners[instance];
            if (entityToIndex.Contains(owner))
            {
                instanceOwners[instance] = entityToIndex[owner];
            }
            else
            {
                // hash entity
                entityToIndex.Add(owner, indexHash);
                instanceOwners[instance] = indexHash;
                indexHash++;
            }
        }

        static_assert(sizeof(Game::Entity) == sizeof(uint32_t), "Flatbuffer owner array inner type is not the same size as Game::Entity. Adjust the flatbuffer file!");
        auto owners = builder.CreateVector(reinterpret_cast<uint32_t*>(instanceOwners.Begin()), instanceOwners.Size());

		// Serialize the component data into builddata local stream
		Ptr<IO::BinaryWriter> bWriter = IO::BinaryWriter::Create();
        Ptr<IO::MemoryStream> mStream = IO::MemoryStream::Create();
		bWriter->SetStream(mStream);
		bWriter->Open();

        if (component->functions.Serialize != nullptr)
			component->functions.Serialize(bWriter);

		bWriter->Close();
		
		auto dataStream = builder.CreateVector((ubyte*)mStream->GetRawPointer(), mStream->GetSize());

        Game::Serialization::ComponentDataBuilder c(builder);
        c.add_fourcc(component->GetIdentifier().AsUInt());
        c.add_numInstances(numInstances);
        c.add_attributes(attributes);
        c.add_owners(owners);
        c.add_dataStream(dataStream);

        componentsData.Append(c.Finish());
	}
    
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

    auto components = builder.CreateVector(componentsData.Begin(), componentsData.Size());
    auto parents = builder.CreateVector(parentIndices.Begin(), parentIndices.Size());
    Game::Serialization::EntityBundleBuilder entitiesBuilder(builder);
    entitiesBuilder.add_numEntities(numEntities);
    entitiesBuilder.add_components(components);
    entitiesBuilder.add_numComponents(numComponents);
    entitiesBuilder.add_parentIndices(parents);
    auto entityBundle = entitiesBuilder.Finish();

    Game::Serialization::LevelBuilder sceneBuilder(builder);
    sceneBuilder.add_entities(entityBundle);
    auto scene = sceneBuilder.Finish();
    // Add file identifier
    Game::Serialization::FinishLevelBuffer(builder, scene);

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
    auto uri = IO::URI(levelName);
    Ptr<IO::FileStream> stream = IO::FileStream::Create();
    stream->SetAccessMode(IO::Stream::AccessMode::ReadAccess);
    stream->SetURI(uri);
    
    if (!stream->Open())
        return false;

    void* fileMap = stream->Map();

    if (!Game::Serialization::LevelBufferHasIdentifier(fileMap))
    {
        n_warning("Incorrect magic number!");
        stream->Unmap();
        stream->Close();
        return false;
    }

    auto level = Game::Serialization::GetLevel(fileMap);
    auto entityBundle = level->entities();
    auto& components = *entityBundle->components();
    
    Util::Array<Game::Entity> entities = Game::EntityManager::Instance()->CreateEntities(entityBundle->numEntities());

    // Create a convenient array to pass to all components
    const Util::Array<uint> parentIndices(entityBundle->parentIndices()->data(), entityBundle->parentIndices()->size());

	// We need to save each component and enitity start index so that we can call activate after
	// all components has been loaded
	Util::Array<Listener> activateListeners;
    
	for (auto component : components)
	{
		Game::ComponentInterface* c = Game::ComponentManager::Instance()->GetComponentByFourCC(component->fourcc());
		if (c != nullptr)
		{
			// Needs to create entirely new instances, not reuse old.
            // This is so that we can actually patch owners and parents.

            auto numInstances = component->numInstances();

			uint start = c->NumRegistered();
			c->Allocate(numInstances);
			uint end = c->NumRegistered();

			Ptr<IO::BinaryReader> bReader = IO::BinaryReader::Create();
            Ptr<IO::MemoryStream> mStream = IO::MemoryStream::Create();

            // TODO: Kind of an unnecessary copy, should create a constant memory stream (memory scanner?) that only allows reading.
            mStream->SetSize(component->dataStream()->size());
            Memory::Copy(component->dataStream()->data(), mStream->GetRawPointer(), component->dataStream()->size());

            bReader->SetStream(mStream);
			bReader->Open();
			
			if (c->functions.Deserialize != nullptr)
				c->functions.Deserialize(bReader, start, numInstances);
			
			bReader->Close(); // automatically closes the memory stream. The copied memory will be freed when the destructor is called (end of scope)

            Util::Array<Game::Entity> newOwners((Game::Entity*)component->owners()->data(), component->owners()->size());
            c->SetOwners(start, newOwners);

			// update owner and id maps
            for (uint j = start; j < end; j++)
            {
                c->SetOwner(j, entities[c->GetOwner(j).id]);
            }
			
			if (c->functions.SetParents != nullptr && parentIndices.Size() > 0)
			{
				c->functions.SetParents(start, end, entities, parentIndices);
			}

			if (c->SubscribedEvents().IsSet(Game::ComponentEvent::OnLoad) && c->functions.OnLoad != nullptr)
			{
				SizeT end = start + numInstances;
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
				listener.numInstances = numInstances;
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

    stream->Unmap();
    stream->Close();

	return true;
}

} // namespace BaseGameFeature
