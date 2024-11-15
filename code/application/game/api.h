#pragma once
//------------------------------------------------------------------------------
/**
    @file api.h

    The main programming interface for the Game Subsystem.

    @copyright
    (C) 2020-2024 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/ptr.h"
#include "category.h"
#include "entity.h"
#include "memdb/tableid.h"
#include "filter.h"
#include "dataset.h"

namespace MemDb
{
class Database;
}

namespace Game
{

class World;
#define WORLD_DEFAULT Game::WorldHash{'DWLD'} // 0x44574C44

/// Destroy a filter
void DestroyFilter(Filter);

/// Query a subset of tables in a specific db using a specified filter set. Modifies the tables array so that it only contains valid tables.
/// This does NOT wait for resources to be available.
Dataset Query(Ptr<MemDb::Database> const& db, Util::Array<MemDb::TableId>& tables, Filter filter);
/// Recycles all current datasets allocated memory to be reused
void ReleaseDatasets();

/// Returns a blueprint id by name
BlueprintId GetBlueprintId(Util::StringAtom name);
/// Returns a template id by name
TemplateId GetTemplateId(Util::StringAtom name);

} // namespace Game
