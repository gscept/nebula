//------------------------------------------------------------------------------
//  api.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "api.h"
#include "gameserver.h"
#include "ids/idallocator.h"
#include "memdb/tablesignature.h"
#include "memdb/database.h"
#include "memory/arenaallocator.h"
#include "basegamefeature/managers/blueprintmanager.h"
#include "profiling/profiling.h"
#include "util/fixedarray.h"

namespace Game
{

static Memory::ArenaAllocator<sizeof(Dataset::EntityTableView) * 256> viewAllocator;

//------------------------------------------------------------------------------
/**
*/
World*
GetWorld(uint32_t hash)
{
    return GameServer::Instance()->GetWorld(hash);
}

//------------------------------------------------------------------------------
/**
*/
void
ReleaseDatasets()
{
    viewAllocator.Release();
}

//------------------------------------------------------------------------------
/**
    @returns    Dataset with category table views.

    @note       The category table view buffer can be NULL if the filter contains
                a non-typed/flag component.
*/
Dataset
Query(World* world, Filter filter)
{
#if NEBULA_ENABLE_PROFILING
    //N_COUNTER_INCR("Calls to Game::Query", 1);
    N_SCOPE_ACCUM(QueryTime, EntitySystem);
#endif
    Ptr<MemDb::Database> db = Game::GetWorldDatabase(world);

    Util::Array<MemDb::TableId> tids = db->Query(GetInclusiveTableMask(filter), GetExclusiveTableMask(filter));

    return Query(world, tids, filter);
}

//------------------------------------------------------------------------------
/**
*/
Dataset
Query(World* world, Util::Array<MemDb::TableId>& tids, Filter filter)
{
    Ptr<MemDb::Database> db = Game::GetWorldDatabase(world);
    return Query(db, tids, filter);
}

//------------------------------------------------------------------------------
/**
*/
Game::Dataset
Query(Ptr<MemDb::Database> const& db, Util::Array<MemDb::TableId>& tids, Filter filter)
{
    Game::Dataset data;
    data.numViews = 0;

    // count num views.
    for (IndexT i = 0; i < tids.Size(); i++)
    {
        MemDb::Table& tbl = db->GetTable(tids[i]);
        SizeT const numRows = tbl.GetNumRows();
        if (numRows > 0)
        {
            data.numViews += tbl.GetNumActivePartitions();
        }
    }

    if (data.numViews == 0)
    {
        data.views = nullptr;
        return data;
    }

    data.views = (Dataset::EntityTableView*)viewAllocator.Alloc(sizeof(Dataset::EntityTableView) * data.numViews);
    data.numViews = 0;

    Util::FixedArray<ComponentId> const& components = ComponentsInFilter(filter);

    for (IndexT tableIndex = 0; tableIndex < tids.Size(); tableIndex++)
    {
        if (db->IsValid(tids[tableIndex]))
        {
            MemDb::Table& tbl = db->GetTable(tids[tableIndex]);
            SizeT const numRows = tbl.GetNumRows();
            if (numRows > 0)
            {
                MemDb::Table::Partition* part = tbl.GetFirstActivePartition();
                while (part != nullptr)
                {
                    Dataset::EntityTableView* view = data.views + data.numViews;
                    view->tableId = tids[tableIndex];

                    IndexT i = 0;
                    for (auto component : components)
                    {
                        MemDb::ColumnIndex colId = tbl.GetAttributeIndex(component);
                        view->buffers[i] = tbl.GetBuffer(part->partitionId, colId);
                        i++;
                    }

                    view->numInstances = part->numRows;
                    view->partitionId = part->partitionId;
                    data.numViews++;
                    part = part->next;
                }
            }
        }
        else
        {
            tids.EraseIndexSwap(tableIndex);
            // re-run the same index
            tableIndex--;
        }
    }

    return data;
}

//------------------------------------------------------------------------------
/**
*/
ComponentId
CreateComponent(ComponentCreateInfo const& info)
{
    ComponentId const component = MemDb::TypeRegistry::Register(info.name, info.byteSize, info.defaultValue, info.flags);
    return component;
}

//------------------------------------------------------------------------------
/**
*/
ComponentId
GetComponentId(Util::StringAtom name)
{
    return MemDb::TypeRegistry::GetComponentId(name);
}

//------------------------------------------------------------------------------
/**
*/
BlueprintId
GetBlueprintId(Util::StringAtom name)
{
    return BlueprintManager::GetBlueprintId(name);
}

//------------------------------------------------------------------------------
/**
*/
TemplateId
GetTemplateId(Util::StringAtom name)
{
    return BlueprintManager::GetTemplateId(name);
}

namespace Internal
{
//------------------------------------------------------------------------------
/**
    Do not use. This function generates a new component id.
*/
uint16_t
GenerateNewComponentId()
{
    static uint16_t idCounter = -1;
    return ++idCounter;
}
}

} // namespace Game
