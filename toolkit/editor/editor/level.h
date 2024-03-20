#pragma once
//------------------------------------------------------------------------------
/**
    @class Editor::Level

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "memdb/database.h"
#include "game/entity.h"

namespace Editor
{

class Level;

struct Sublevel
{
    Util::String alias;
    
    Math::vec3 position;
    Math::quat orientation;
    Math::vec3 scale;

    Ptr<Level> level;
};

/*

TODO:

Level util in toolkit:
parses and converts json level into binary.
Binary is in table format, but requires some additional bookkeeping of which attributes are in the table, which size and what fields they have
We should also have "additional data" that can be appended to each entity (such as script data)

*/

class Level : public Core::RefCounted
{
    __DeclareClass(Level);

public:
    // constructor
    Level();
    /// destructor
    virtual ~Level();

    /// clear the whole level
    void Clear();
    /// Loads a level from an json file
    bool LoadLevel(const Util::String& name);

    /// Save level with different name, name is only basename, without path or extension
    bool SaveLevelAs(const Util::String& name);

    /// export to nlvl format
    static bool Export(const Util::String& name);

    /// get level name
    const Util::String& GetName() const;
    
    ///
    void RemoveReference(const Util::String& level);

private:
    /// level name
    Util::String name;
    Util::Array<Game::Entity> entities; // the entities in the level db
};

} // namespace Editor
