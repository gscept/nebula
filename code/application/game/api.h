#pragma once
//------------------------------------------------------------------------------
/**
    @file   api.h

    The main programming interface for the Game Subsystem.

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
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

/*
        Make it so that we can add a processor OnCreate that runs directly on newly created instances.
        So that we can initialize stuff in OnCreate, then run OnActivate

        OnCreate is executed directly for entities when created

        OnCreate can be a ComponentEvent, an event dispatcher. The table knows its signature, and we want to be able to create listeners based on those signatures and the component event.

        Frame events should be objects that are looked up by name. They should have a sorting order as well

        Structure the entire system. Currently, everything is a bit spaghetti, which makes it hard to reason about.
        Take all the concepts, lay them out on a table, draw lines, connect the systems, see if we can simplify some of it.

        Maybe do a whole refactor, from scratch.
            Rip out section by section into new files, until everything has been covered.
    */

//------------------------------------------------------------------------------

#define WORLD_DEFAULT uint32_t('DWLD') // 0x44574C44

class World;

/// returns a world by hash
World* GetWorld(uint32_t worldHash);

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
