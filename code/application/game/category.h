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

struct EntityCreateInfo
{
	CategoryId category;
};

/// describes a category
struct Category
{
	Util::StringAtom name;
	MemDb::TableId instanceTable;
	MemDb::TableId templateTable;
};

struct EntityMapping
{
	CategoryId category;
	InstanceId instance;
};

typedef MemDb::TableCreateInfo CategoryCreateInfo;

} // namespace Game
