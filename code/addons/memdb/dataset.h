#pragma once
//------------------------------------------------------------------------------
/**
    @class	MemDb::Dataset

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "propertyid.h"

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
        Util::ArrayStack<void*, 16> buffers;
    };

    /// views into the tables
    Util::Array<View> tables;

    /// Validate tables and num instances. This ensures valid data, but does not re-query for new tables.
    void Validate();
private:
    friend class Database;
    /// pointer to the database used to construct this set
    Ptr<Database> db;
};

} // namespace MemDb
