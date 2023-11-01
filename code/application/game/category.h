#pragma once
//------------------------------------------------------------------------------
/**
    @file   category.h

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/string.h"
#include "util/fixedarray.h"
#include "component.h"

namespace Game
{

ID_16_TYPE(BlueprintId);
ID_32_TYPE(TemplateId);

struct CategoryCreateInfo
{
    /// name to be given to the table
    Util::String name;
    /// which properties the table should have
    Util::FixedArray<ComponentId> components;
};

} // namespace Game
