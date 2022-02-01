//------------------------------------------------------------------------------
//  @file dataset.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "dataset.h"
#include "database.h"

namespace MemDb
{

//------------------------------------------------------------------------------
/**
*/
void
Dataset::Validate()
{
    for (IndexT i = 0; i < this->tables.Size();)
    {
        auto& view = this->tables[i];
        if (!db->IsValid(view.tid))
        {
            this->tables.EraseIndexSwap(i);
            continue;
        }

        view.numInstances = db->GetNumRows(view.tid);
        i++;
    }
}

} // namespace MemDb
