//------------------------------------------------------------------------------
//  @file table.cc
//  @copyright (C) 2023 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "table.h"
namespace MemDb
{

//------------------------------------------------------------------------------
/**
*/
bool
_Table::HasAttribute(AttributeId attribute) const
{
    return this->signature.IsSet(attribute);
}

//------------------------------------------------------------------------------
/**
*/
AttributeId
_Table::GetAttributeId(ColumnIndex columnIndex) const
{
    return this->attributes[columnIndex.id];
}

} // namespace MemDb
