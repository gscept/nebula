#pragma once
//------------------------------------------------------------------------------
/**
    @class Level

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
        byte* columns = nullptr;
    };

    Util::Array<EntityGroup> tables;
};

} // namespace Game
