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

struct Category
{
    MemDb::TableId instanceTable;
    CategoryHash hash;
#ifdef NEBULA_DEBUG
    Util::String name;
#endif
};

typedef MemDb::TableCreateInfo CategoryCreateInfo;

} // namespace Game
