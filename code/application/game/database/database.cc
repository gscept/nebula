//------------------------------------------------------------------------------
//  database.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "database.h"

namespace Game
{
__ImplementClass(Game::Database, 'GMDB', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
Database::Database()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Database::~Database()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
TableId
Database::CreateTable(TableCreateInfo const& info)
{
    TableId id;
    this->tableIdPool.Allocate(id.id);
    n_assert(Ids::Index(id.id) <= this->tables.Size());

    Table* table;

    if (Ids::Index(id.id) == this->tables.Size())
    {
        uint32_t idx = this->tables.Alloc();
        table = &this->tables.Get<0>(idx);
    }
    else
    {
        table = &this->tables.Get<0>(Ids::Index(id.id));
    }

    table->name = info.name;
    table->columns.Clear();
    for (IndexT i = 0; i < info.columns.Size(); i++)
    {
        this->AddColumn(id, info.columns[i]);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Database::DeleteTable(TableId table)
{
    n_delete(this->tables[Ids::Index(table.id)]);
    this->tableIdPool.Deallocate(table.id);

}

//------------------------------------------------------------------------------
/**
*/
bool
Database::IsValid(TableId table)
{
    return this->tableIdPool.IsValid(table.id);
}

//------------------------------------------------------------------------------
/**
*/
Column
Database::GetColumn(TableId table, ColumnId columnId)
{
    return this->tables[Ids::Index(table.id)]->columns[columnId.id];
}

//------------------------------------------------------------------------------
/**
*/
ColumnId
Database::GetColumnId(TableId table, Column column)
{
    ColumnId cid = this->tables[Ids::Index(table.id)]->columns.FindIndex(column);
    n_assert(cid != ColumnId::Invalid());
    return cid;
}

//------------------------------------------------------------------------------
/**
*/
ColumnId
Database::AddColumn(TableId table, Column column)
{
    this->tables[Ids::Index(table.id)]->columns.Append(column);
    
}


} // namespace Game
