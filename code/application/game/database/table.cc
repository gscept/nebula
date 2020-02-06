//------------------------------------------------------------------------------
//  table.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "application/stdneb.h"
#include "table.h"
namespace Game
{

__ImplementClass(Game::Table, 'Gtbl', Core::RefCounted)

//------------------------------------------------------------------------------
/**
*/
Table::Table()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Table::~Table()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
ColumnId
Table::AddColumn(Column attrid)
{
    n_assert2(this->columns.FindIndex(attrid) == InvalidIndex, "Can only add one column of each type!");

    this->columns.Append(attrid);
    this->columnIds.Add(attrid, this->columns.Size() - 1);
}

//------------------------------------------------------------------------------
/**
*/
Column
Table::GetColumnType(ColumnId col)
{
    return this->columns[col.id];
}

//------------------------------------------------------------------------------
/**
*/
ColumnId
Table::FindColumn(Column attrid)
{
    return this->columns.FindIndex(attrid);
}
} // namespace Game
