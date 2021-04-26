#pragma once
//------------------------------------------------------------------------------
/**
    @file   category.h

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "memdb/propertyid.h"
#include "util/string.h"
#include "util/fixedarray.h"

namespace Game
{

typedef MemDb::PropertyId PropertyId;

ID_16_TYPE(BlueprintId);
ID_32_TYPE(TemplateId);

struct CategoryCreateInfo
{
    /// name to be given to the category
    Util::String name;
    /// which properties the category should have
    Util::FixedArray<PropertyId> properties;
};

} // namespace Game
