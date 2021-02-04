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

/// category hash
struct CategoryHash
{
    uint32_t id = 0;

    const bool operator==(CategoryHash const rhs) const
    {
        return id == rhs.id;
    }
    const bool operator!=(CategoryHash const rhs) const
    {
        return id != rhs.id;
    }
    const bool operator<(CategoryHash const rhs) const
    {
        return HashCode() < rhs.HashCode();
    }
    const bool operator>(CategoryHash const rhs) const
    {
        return HashCode() > rhs.HashCode();
    }

    void AddToHash(uint32_t i)
    {
        this->id += i;
        this->id = Hash(this->id);
    }
    void RemoveFromHash(uint32_t i)
    {
        this->id = UnHash(this->id);
        this->id -= i;
    }
    static uint32_t Hash(uint32_t i)
    {
        i = ((i >> 16) ^ i) * 0x45d9f3b;
        i = ((i >> 16) ^ i) * 0x45d9f3b;
        i = (i >> 16) ^ i;
        return i;
    }
    static uint32_t UnHash(uint32_t i)
    {
        i = ((i >> 16) ^ i) * 0x119de1f3;
        i = ((i >> 16) ^ i) * 0x119de1f3;
        i = (i >> 16) ^ i;
        return i;
    }
    uint32_t HashCode() const
    {
        return id;
    }
};

//------------------------------------------------------------------------------
/**
    @struct Game::Category

    @internal There are some assumptions made about categories that should always
    be kept in mind:
        1. Categories NEVER change signature, meaning when the category has been
        set up, we can never remove or add properties to it. In that case, create
        a new category.
*/
struct Category
{
    /// instance table contains all instances of this category
    MemDb::TableId instanceTable;
    /// hash identifier for this specific category
    CategoryHash hash;
    /// contains only the managed property columns that are in the instance table
    /// entities that are destroyed are moved to this table, so that manager have a
    /// chance of cleaning up any externally allocated resources.
    MemDb::TableId managedPropertyTable;

#ifdef NEBULA_DEBUG
    Util::String name;
#endif
};

struct CategoryCreateInfo
{
    /// name to be given to the category
    Util::String name;
    /// which properties the category should have
    Util::FixedArray<PropertyId> properties;
};

} // namespace Game
