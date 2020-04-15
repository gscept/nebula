#pragma once
//------------------------------------------------------------------------------
/**
    @file	category.h

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "entity.h"
#include "game/database/table.h"
#include "core/refcounted.h"

namespace Game
{

class Property;

struct FilterSet
{
	/// categories must include all attributes in this array
	Util::Array<Game::AttributeId> inclusive;
	/// categories must NOT contain any attributes in this array
	Util::Array<Game::AttributeId> exclusive;
};

struct Dataset
{
	/// A view into a category table.
	struct View
	{
		CategoryId cid;
		SizeT numInstances = 0;
		Util::ArrayStack<void*, 16> buffers;
	};

	/// the filter that has been used to attain this dataset
	FilterSet filter;

	/// views into the categories
	Util::Array<View> categories;
};

struct EntityCreateInfo
{
	CategoryId category;
	Util::FixedArray<Attribute> attributes;
};

/// describes a category
struct Category
{
	Util::StringAtom name;
	Db::TableId instanceTable;
	Db::TableId templateTable;
	Util::Array<Ptr<Game::Property>> properties;
};

struct EntityMapping
{
	CategoryId category;
	InstanceId instance;
};

typedef Game::Db::TableCreateInfo CategoryCreateInfo;

} // namespace Game
