#pragma once
//------------------------------------------------------------------------------
/**
    @class BaseGameFeature::LevelParser
    
    Base class for level parsing

    Entity format is defined as:
    \code{.json}
    {
        "level": {
            "version": "100"
            "entities": {
                "[GUID]": {
                    "name": String,
                    "components": {
                        "ComponentName1": VALUE,
                        "ComponentName2": {
                            "FieldName1": VALUE,
                            "FieldName2": VALUE
                        }
                    },
                },
                "[GUID]": {
                    ...
                }
            }
        }
    }
    \endcode

    @todo   This class is a (working) mess right now, and should be refactored.
            An idea is to first load the json into an intermediate format/struct
            that contains all the loaded entities, not as actual entities but as smaller
            objects with kvp properties. This would require us to rewrite serialization
            somewhat, but we might gain quite a bit of flexibility from it if we move away
            from the json reader directly. This would be similar to how we parse and use 
            gltfs. This would also make it easier to load subscenes, as they would be parsed
            directly into this structure/format.

    
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "io/jsonreader.h"
#include "game/entity.h"
#include "util/guid.h"
#include "util/hashtable.h"

namespace Game
{
class World;
}

namespace BaseGameFeature
{

class LevelParser : public Core::RefCounted
{
    __DeclareClass(LevelParser);
public:
    /// constructor
    LevelParser();
    /// destructor
    virtual ~LevelParser();

    /// Set the world that we want to load the levels into
    void SetWorld(Game::World* world);

    /// Loads a level from a json file in work:levels.
    Util::Array<Game::Entity> LoadJsonLevel(const Ptr<IO::JsonReader>& reader);

protected:
    /// parse a single object
    Game::Entity LoadEntity(const Ptr<IO::JsonReader> & reader);
    /// parse all components for an entity
    void LoadComponents(const Ptr<IO::JsonReader>& reader, Game::Entity entity);
    /// called at beginning of load
    virtual void BeginLoad(){}
    /// add entity
    virtual void AddEntity(Game::Entity entity, Util::Guid const& guid){}
    /// set entity name
    virtual void SetName(Game::Entity entity, const Util::String & name){}
    /// entity loaded completely
    virtual void CommitEntity(Game::Entity entity) {}
    /// parsing done
    virtual void CommitLevel(){}

private:
    Util::Array<Util::String> invalidAttrs;
    Util::HashTable<Util::Guid, Game::Entity> guidToEntity;
    Game::World* world;
}; 

} // namespace BaseGameFeature
//------------------------------------------------------------------------------
