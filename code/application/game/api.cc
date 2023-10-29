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

static Memory::ArenaAllocator<sizeof(Dataset::View) * 256> viewAllocator;

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

    data.views = (Dataset::View*)viewAllocator.Alloc(sizeof(Dataset::View) * data.numViews);
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
                    Dataset::View* view = data.views + data.numViews;
                    view->tableId = tids[tableIndex];
                    view->validInstances = part->validRows;

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

} // namespace Game
