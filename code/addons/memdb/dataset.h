#pragma once
//------------------------------------------------------------------------------
/**
    @class  MemDb::Dataset

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "componentid.h"
#include "memdb/table.h"

namespace MemDb
{

class Database;

struct Dataset
{
    /// A view into a category table.
    struct View
    {
#ifdef NEBULA_DEBUG
        Util::String tableName;
#endif
        TableId tid;
        SizeT numInstances = 0;
        Util::StackArray<void*, 16> buffers;
    };

    /// views into the tables
    Util::Array<View> tables;

private:
    friend class Database;
    /// pointer to the database used to construct this set
    Ptr<Database> db;
};

} // namespace MemDb
