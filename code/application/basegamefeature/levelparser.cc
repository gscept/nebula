//------------------------------------------------------------------------------
//  levelparser.cc
//  (C) 2015-2024 Individual contributors, see AUTHORS file
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
    this->BeginLoad();

    reader->SetToRoot();

    if (reader->SetToNode("/level"))
    {
        int levelversion = reader->GetInt("version");
        if (levelversion != 100)
        {
            n_warning("Unsupport level version!");
            return {};
        }

        // Now read all entities that exists in this scene
        reader->SetToFirstChild("entities");
        
        this->invalidAttrs.Clear();

        Util::Array<Game::Entity> entities;

        reader->SetToFirstChild();
        do 
        {
            if (reader->HasAttr("sub_scene"))
            {
                // 1. Load subscene consisting of multiple entities.
                // 2. Group them in editor
                //    Maybe this can be made with a sort of hierarchical "Transform" component (changes to this entitys transform is propagated to it's children, and the children needs to have a "parent" component, consisting of local pos, rot, scale and a parent ID)
                // 3. Tie them to the scene resource, so that if the resource is
                //    updated, the entities are as well
            }
            else // regular entity, just load normally
            {
                Game::Entity entity = this->LoadEntity(reader);
                if (entity != Game::Entity::Invalid())
                    entities.Append(entity);
            }

        }
        while(reader->SetToNextChild());

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
            errorMessage.Format("\n\nInvalid components (have they been removed since last level save?) has been found in level '%s':\n\n", levelName.AsCharPtr());

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
        
        return entities;
    }

    return Util::Array<Game::Entity>();
}

//------------------------------------------------------------------------------
/**
*/
Game::Entity
LevelParser::LoadEntity(const Ptr<IO::JsonReader> & reader)
{
    Util::String guid = reader->GetCurrentNodeName();
    
    Util::String entityName = reader->GetOptString("name", "unnamed_entity");

    Game::Entity entity = world->CreateEntity(true);
    
    this->AddEntity(entity, Util::Guid::FromString(guid));
    this->SetName(entity, entityName);

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

    return entity;
}

} // namespace BaseGameFeature
