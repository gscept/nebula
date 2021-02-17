#pragma once
//------------------------------------------------------------------------------
/**
    @file   category.h

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "entity.h"
#include "core/refcounted.h"
#include "memdb/propertyid.h"
#include "memdb/table.h"
#include "memdb/database.h"

namespace Game
{

typedef MemDb::PropertyId PropertyId;

ID_16_TYPE(BlueprintId);
ID_16_16_NAMED_TYPE(TemplateId, blueprintId, templateId);

struct CategoryCreateInfo
{
    /// name to be given to the category
    Util::String name;
    /// which properties the category should have
    Util::FixedArray<PropertyId> properties;
};

} // namespace Game
