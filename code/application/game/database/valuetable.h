#pragma once
//------------------------------------------------------------------------------
/**
    ValueTable

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/id.h"

namespace Game
{

ID_32_TYPE(ValueTableId);

struct ValueTable
{
    void* columns[MAX_VALUE_TABLE_COLUMNS];
    int numColumns = 0;
    int numRows = 0;
};

} // namespace Game
