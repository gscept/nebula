#pragma once
//------------------------------------------------------------------------------
/**
    @file   category.h

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "memdb/componentid.h"
#include "util/string.h"
#include "util/fixedarray.h"

namespace Game
{

typedef MemDb::ComponentId ComponentId;

ID_16_TYPE(BlueprintId);
ID_32_TYPE(TemplateId);

#define DECLARE_COMPONENT public:\
static Game::ComponentId ID() { return id; }\
private:\
    friend class MemDb::TypeRegistry;\
    static Game::ComponentId id;\
public:

#define DEFINE_COMPONENT(TYPE) Game::ComponentId TYPE::id = Game::ComponentId::Invalid();

struct CategoryCreateInfo
{
    /// name to be given to the table
    Util::String name;
    /// which properties the table should have
    Util::FixedArray<ComponentId> components;
};

} // namespace Game
