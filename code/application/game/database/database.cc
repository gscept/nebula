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
    n_error("implement me!");
    //n_delete(this->tables[Ids::Index(table.id)]);
    //this->tableIdPool.Deallocate(table.id);

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
    return this->tables.Get<0>(Ids::Index(table.id)).columns.Get<0>(columnId.id);
}

//------------------------------------------------------------------------------
/**
*/
ColumnId
Database::GetColumnId(TableId table, Column column)
{
    ColumnId cid = this->tables.Get<0>(Ids::Index(table.id)).columns.GetArray<0>().FindIndex(column);
    n_assert(cid != ColumnId::Invalid());
    return cid;
}

//------------------------------------------------------------------------------
/**
*/
ColumnId
Database::AddColumn(TableId tid, Column column)
{
    Game::Database::Table& table = this->tables.Get<0>(Ids::Index(tid.id));
    uint32_t col = table.columns.Alloc();
    
    Table::ColumnData buffer = table.columns.Get<1>(col);
    table.columns.Get<0>(col) = column;

    //  TODO: Get size from the attribute
    const SizeT byteSize = 16;//Attr::SizeOf(column);
    buffer = Memory::Alloc(Table::HEAP_MEMORY_TYPE, table.capacity * byteSize);

    // TODO: Fill buffer with default values
    // Memory::Fill(buffer, table.numRows, 0);

}

//------------------------------------------------------------------------------
/**
*/
IndexT
Database::AllocateRow(TableId tid)
{
    Game::Database::Table& table = this->tables.Get<0>(Ids::Index(tid.id));
    auto& colTypes = table.columns.GetArray<0>();
    auto& buffers = table.columns.GetArray<1>();

    int row = table.numRows++;

    if (table.numRows < table.capacity)
    {
        // Just set the default values
        for (IndexT i = 0; i < buffers.Size(); i++)
        {
            ((char*)buffers[i])
        }
    }
    else 
    {
        // We need to grow the buffers
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Database::DeallocateRow(TableId table, IndexT row)
{

}

} // namespace Game
