#pragma once
//------------------------------------------------------------------------------
/**
    @file	category.h

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
typedef MemDb::Dataset Dataset;
typedef MemDb::FilterSet FilterSet;

ID_16_TYPE(BlueprintId);
ID_16_16_NAMED_TYPE(TemplateId, blueprintId, templateId);

struct EntityCreateInfo
{
	BlueprintId blueprint = BlueprintId::Invalid();
	TemplateId templateId = TemplateId::Invalid();
	bool immediate = false;
};

struct Category
{
	MemDb::TableId instanceTable;
	CategoryHash hash;
#ifdef NEBULA_DEBUG
	Util::String name;
#endif
};

struct EntityMapping
{
	CategoryId category;
	InstanceId instance;
};

typedef MemDb::TableCreateInfo CategoryCreateInfo;



} // namespace Game
