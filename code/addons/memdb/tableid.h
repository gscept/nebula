#pragma once
//------------------------------------------------------------------------------
/**
    @file tableid.h

    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
namespace MemDb
{

/// Table identifier
ID_32_TYPE(TableId);

/// row identifier
struct RowId
{
    uint16_t partition;
    uint16_t index;

    bool
    operator!=(RowId const& rhs)
    {
        return partition != rhs.partition || index != rhs.index;
    }

    bool
    operator==(RowId const& rhs)
    {
        return partition == rhs.partition && index == rhs.index;
    }
};

constexpr RowId InvalidRow = {0xFFFF, 0xFFFF};

/// column id
ID_16_TYPE(ColumnIndex);

} // namespace MemDb
