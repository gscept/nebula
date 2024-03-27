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
    
    (C) 2015-2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "io/jsonreader.h"
#include "game/entity.h"
#include "util/guid.h"

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
    bool LoadJsonLevel(const Ptr<IO::JsonReader> & reader);

protected:
    /// parse a single object
    Game::Entity LoadEntity(const Ptr<IO::JsonReader> & reader);
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
    Game::World* world;
}; 

} // namespace BaseGameFeature
//------------------------------------------------------------------------------
