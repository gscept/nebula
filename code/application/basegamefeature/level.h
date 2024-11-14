#pragma once
//------------------------------------------------------------------------------
/**
    @file level.h

    @copyright
    (C) 2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "memdb/database.h"
#include "game/entity.h"

namespace Game
{

class World;

//------------------------------------------------------------------------------
/**
    @class Game::PackedLevel

    @brief Represents a level that can be instantiated into a world.

    @details A packed level contains all information of a level
    in memory-packed entity groups that correspont to a table in the 
    game world. When instantiating, the entity groups are effectively
    just mem-copied into the game world, and then initialized.

    PackedLevels are loaded directly via a game world and should not
    be created using `new`.

    @see Game::World::PreloadLevel
    @see Game::World::UnloadLevel
    
*/
class PackedLevel
{
public:
    /// instantiates the level into game world
    Util::Array<Game::Entity> Instantiate() const;

private:
    friend class World;

    PackedLevel() {}; // only worlds may create this

    // only worlds may destroy this
    ~PackedLevel()
    {
        for (auto& t : tables)
        {
            if (t.columns != nullptr)
                delete[] t.columns;
        }
    };

    // the destination world if we are to instantiate this level
    Game::World* world;

    struct EntityGroup
    {
        MemDb::TableId dstTable;
        SizeT numRows;
        ubyte* columns = nullptr;
    };

    Util::Array<EntityGroup> tables;
};

} // namespace Game
