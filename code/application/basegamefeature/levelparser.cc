//------------------------------------------------------------------------------
//  levelparser.cc
//  (C) 2024 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "levelparser.h"
#include "game/world.h"

using namespace Util;
using namespace Math;

namespace BaseGameFeature
{

__ImplementClass(BaseGameFeature::LevelParser, 'LVPR', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
LevelParser::LevelParser()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
LevelParser::~LevelParser()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
LevelParser::SetWorld(Game::World* world)
{
    this->world = world;
}

//------------------------------------------------------------------------------
/**
*/
Util::Array<Game::Entity>
LevelParser::LoadJsonLevel(const Ptr<IO::JsonReader> & reader)
{
    if (this->world == nullptr || !reader.isvalid())
    {
        n_warning("LevelParser::LoadJsonLevel requires a world and a valid reader\n");
        return {};
    }

    reader->SetToRoot();
    if (!reader->SetToNode("/level"))
    {
        n_warning("LevelParser::LoadJsonLevel could not find the level root\n");
        return {};
    }

    int const levelversion = reader->GetInt("version");
    if (levelversion != 100)
    {
        n_warning("Unsupported level version: %d\n", levelversion);
        return {};
    }

    this->guidToEntity.Clear();
    this->invalidAttrs.Clear();

    auto& g2e = this->guidToEntity;
    Game::ComponentSerialization::OverrideType(
        Game::ComponentSerialization::ENTITY,
        [&g2e](Ptr<IO::JsonReader> const& reader, const char* name, void* data)
        {
            Util::Guid guid;
            reader->Get<Util::Guid>(guid, name);
            IndexT index = g2e.FindIndex(guid);
            // Make sure the entity is already setup.
            n_assert(index != InvalidIndex);
            *(Game::Entity*)data = g2e.ValueAtIndex(guid, index);
        },
        nullptr
    );

    this->BeginLoad();
    Util::Array<Game::Entity> entities;

    if (reader->SetToFirstChild("entities") && reader->SetToFirstChild())
    {
        // Load entities first so component entity references can resolve forwards.
        this->guidToEntity.BeginBulkAdd();
        do
        {
            if (reader->HasAttr("sub_scene"))
            {
                // Sub-scenes are reserved for future nested level support.
            }
            else
            {
                entities.Append(this->LoadEntity(reader));
            }
        } while (reader->SetToNextChild());
        this->guidToEntity.EndBulkAdd();

        // Deserialize components after every source GUID has a runtime entity.
        reader->SetToFirstChild();
        IndexT entityIndex = 0;
        do
        {
            if (!reader->HasAttr("sub_scene"))
            {
                this->LoadComponents(reader, entities[entityIndex++]);
            }
        } while (reader->SetToNextChild());
    }

    if (!this->invalidAttrs.IsEmpty())
    {
        Util::String levelName;
        if (reader->HasStream())
        {
            levelName = reader->GetStream()->GetURI().LocalPath().ExtractFileName();
            levelName.StripFileExtension();
        }
        Util::String errorMessage;
        errorMessage.Format("\nInvalid components found in level '%s':\n", levelName.AsCharPtr());
        for (IndexT i = 0; i < invalidAttrs.Size(); i++)
        {
            errorMessage.Append("\t" + invalidAttrs[i] + "\n");
        }
        n_warning(errorMessage.AsCharPtr());
    }

    world->ExecuteAddComponentCommands();

    for (Game::Entity entity : entities)
    {
        this->CommitEntity(entity);
    }
    this->CommitLevel();

    Game::ComponentSerialization::OverrideType(Game::ComponentSerialization::ENTITY, nullptr, nullptr);

    return entities;
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
LevelParser::LoadEntity(const Ptr<IO::JsonReader> & reader)
{
    Util::Guid guid = Util::Guid::FromString(reader->GetCurrentNodeName());
    
    Util::String entityName = reader->GetOptString("name", "unnamed_entity");

    Game::Entity entity = world->CreateEntity(true);
    
    this->AddEntity(entity, guid);
    this->SetName(entity, entityName);

    this->guidToEntity.Add(guid, entity);

    return entity;
}

//------------------------------------------------------------------------------
/**
*/
void
LevelParser::LoadComponents(const Ptr<IO::JsonReader>& reader, Game::Entity entity)
{
    Util::String entityName = reader->GetOptString("name", "unnamed_entity");

    if (reader->SetToFirstChild("components"))
    {
        uint numChildren = reader->CurrentSize();
        for (uint childIndex = 0; childIndex < numChildren; childIndex++)
        {
            Util::String const componentName = reader->GetChildNodeName(childIndex);
            Game::ComponentId componentId = Game::GetComponentId(componentName);
            if (componentId == Game::ComponentId::Invalid())
            {
                invalidAttrs.Append(entityName + " ->\t" + componentName);
                continue;
            }

            SizeT const typeSize = MemDb::AttributeRegistry::TypeSize(componentId);
            if (typeSize > 0)
            {
                Util::Blob componentData(MemDb::AttributeRegistry::DefaultValue(componentId), typeSize);
                Game::ComponentSerialization::Deserialize(reader, componentId, componentData.GetPtr());
                world->AddComponent(entity, componentId, componentData.GetPtr());
            }
            else
            {
                world->AddComponent(entity, componentId);
            }
        }

        reader->SetToParent();
    }
}

} // namespace BaseGameFeature
