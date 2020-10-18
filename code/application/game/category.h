#pragma once
//------------------------------------------------------------------------------
/**
    @file	category.h

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "entity.h"
#include "core/refcounted.h"
#include "memdb/columndescriptor.h"
#include "memdb/table.h"

namespace Game
{

typedef MemDb::ColumnDescriptor PropertyId;

ID_16_TYPE(BlueprintId);
ID_16_TYPE(TemplateId);

struct EntityCreateInfo
{
	BlueprintId blueprint = BlueprintId::Invalid();
	TemplateId templateId = TemplateId::Invalid();
};

struct Category
{
	MemDb::TableId instanceTable;
	CategoryHash hash;
};

struct EntityMapping
{
	CategoryId category;
	InstanceId instance;
};

typedef MemDb::TableCreateInfo CategoryCreateInfo;

} // namespace Game
