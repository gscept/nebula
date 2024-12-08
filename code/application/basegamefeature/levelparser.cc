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
    auto& g2e = this->guidToEntity;
    Game::ComponentSerialization::OverrideType( // TODO: this should be a temporary object that is destroyed at end of scope, removing the override
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

    reader->SetToRoot();
    Util::Array<Game::Entity> entities;

    if (reader->SetToNode("/level"))
    {
        int levelversion = reader->GetInt("version");
        if (levelversion != 100)
        {
            n_warning("Unsupport level version!");
            return {};
        }
        else
        {
            // Now read all entities that exists in this scene
            reader->SetToFirstChild("entities");

            this->invalidAttrs.Clear();

            // load entities, setup guid->entity map
            reader->SetToFirstChild();
            this->guidToEntity.BeginBulkAdd();
            do
            {
                if (reader->HasAttr("sub_scene"))
                {
                    // 1. Load subscene consisting of multiple entities.
                    // 2. Group them in editor
                    //    Maybe this can be made with a sort of hierarchical "Transform"
                    //    component (changes to this entitys transform is propagated to
                    //    it's children, and the children needs to have a "parent"
                    //    component, consisting of local pos, rot, scale and a parent ID)
                    // 3. Tie them to the scene resource, so that if the resource is
                    //    updated, the entities are as well
                }
                else // regular entity, just load normally
                {
                    Game::Entity entity = this->LoadEntity(reader);
                    entities.Append(entity);
                }

            } while (reader->SetToNextChild());
            this->guidToEntity.EndBulkAdd();

            // Load components. We do this separately since the entities and their guid
            // needs to be established to be able to patch from GUID to entity in component fields
            reader->SetToFirstChild();
            IndexT entityIndex = 0;
            do
            {
                if (reader->HasAttr("sub_scene"))
                {
                    // Make sure to load all sub_scene entities data
                }
                else // regular entity, just load normally
                {
                    Game::Entity entity = entities[entityIndex++];
                    this->LoadComponents(reader, entity);
                }

            } while (reader->SetToNextChild());

            if (!this->invalidAttrs.IsEmpty())
            {
                Util::String levelName;
                if (reader->HasStream())
                {
                    Util::String fileName = reader->GetStream()->GetURI().LocalPath().ExtractFileName();
                    fileName.StripFileExtension();
                    levelName = fileName;
                }
                // throw an error message telling which components that are missing
                Util::String errorMessage;
                errorMessage.Format(
                    "\n\nInvalid components (have they been removed since last level save?) has been found in level '%s':\n\n",
                    levelName.AsCharPtr()
                );

                for (IndexT i = 0; i < invalidAttrs.Size(); i++)
                {
                    errorMessage.Append("\t" + invalidAttrs[i] + "\n");
                }

                n_warning(errorMessage.AsCharPtr());
            }

            world->ExecuteAddComponentCommands();

            for (IndexT i = 0; i < entities.Size(); i++)
            {
                Game::Entity entity = entities[i];
                this->CommitEntity(entity);
            }

            this->CommitLevel();
        }
    }

    Game::ComponentSerialization::
        OverrideType(
            Game::ComponentSerialization::ENTITY,
            nullptr,
            nullptr
        );

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

            void* componentData = world->AddComponent(entity, componentId);
            Game::ComponentSerialization::Deserialize(reader, componentId, componentData);
        }

        reader->SetToParent();
    }
}

} // namespace BaseGameFeature
