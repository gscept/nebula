#pragma once
//------------------------------------------------------------------------------
/**
    Game::Table

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"
#include "attr/attrid.h"

namespace Game
{


/// TableId contains an id reference to the database it's attached to, and the id of the table.
ID_32_TYPE(TableId);

constexpr uint16_t MAX_VALUE_TABLE_COLUMNS = 128;

/// column id
ID_16_TYPE(ColumnId);

typedef Attr::AttrId Column;

/// Immutable column
//template<typename T>
//struct ColumnData
//{
//    const int& count;
//    T const* buffer;
//};



} // namespace Game
