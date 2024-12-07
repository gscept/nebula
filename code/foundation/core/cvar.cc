//------------------------------------------------------------------------------
//  cvar.cc
//  @copyright (C) 2021$ Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
//
#include "cvar.h"
#include "util/hashtable.h"
#include "util/string.h"

namespace Core
{

struct CVarValue
{
    CVarType type;
    union {
        int i;
        float f;
        char* cstr;
    };
};

struct CVar
{
    Util::String name;
    Util::String description;
    CVarValue value;
    bool modified;
};

constexpr uint16_t MAX_CVARS = 1024;
uint16_t cVarOffset = 0;
CVar cVars[MAX_CVARS];
Util::HashTable<Util::String, uint16_t> cVarTable;

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarCreate(CVarCreateInfo const& info)
{
    CVar* ptr = CVarGet(info.name);
    const bool needsInit = (ptr == nullptr);
    if (ptr == nullptr)
    {
        IndexT varIndex = cVarOffset++;
        n_assert(varIndex < MAX_CVARS);
        ptr = &cVars[varIndex];
        cVarTable.Add(info.name, varIndex);
    }
    n_assert2(!Util::String(info.name).ContainsCharFromSet(" "), "CVar name cannot contain spaces.");
        
    ptr->name = info.name;
    ptr->value.type = info.type;
    ptr->modified = false;
    if (info.description != nullptr)
    {
        ptr->description = info.description;
    }
    if (needsInit)
    {
        CVarParseWrite(ptr, info.defaultValue);
    }
    return ptr;
}

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarCreate(CVarType type, const char* name, const char* defaultValue, const char* description)
{
    CVarCreateInfo info;
    info.name = name;
    info.defaultValue = defaultValue;
    info.type = type;
    info.description = description;
    return CVarCreate(info);
}

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarGet(const char* name)
{
    Util::String const str = name;
    IndexT const index = cVarTable.FindIndex(str);
    if (index != InvalidIndex)
    {
        return &cVars[cVarTable.ValueAtIndex(str, index)];
    }

    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
void
CVarParseWrite(CVar* cVar, const char* value)
{
    switch (cVar->value.type)
    {
    case CVar_Int:
        CVarWriteInt(cVar, atoi(value));
        break;
    case CVar_Float:
        CVarWriteFloat(cVar, atof(value));
        break;
    case CVar_String:
        CVarWriteString(cVar, value);
        break;
    default:
        break;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
CVarWriteFloat(CVar* cVar, float value)
{
    if (cVar->value.type == CVar_Float)
    {
        cVar->value.f = value;
        cVar->modified = true;
        return;
    }

    n_printf("Warning: Invalid CVar type passed to CVarWrite.\n");
    return;
}

//------------------------------------------------------------------------------
/**
*/
void
CVarWriteInt(CVar* cVar, int value)
{
    if (cVar->value.type == CVar_Int)
    {
        cVar->value.i = value;
        cVar->modified = true;
        return;
    }

    n_printf("Warning: Invalid CVar type passed to CVarWrite.\n");
    return;
}

//------------------------------------------------------------------------------
/**
*/
void
CVarWriteString(CVar* cVar, const char* value)
{
    if (cVar->value.type == CVar_String)
    {
        if (cVar->value.cstr)
            Memory::Free(Memory::HeapType::StringDataHeap, cVar->value.cstr);
        
        cVar->value.cstr = Memory::DuplicateCString(value);
        cVar->modified = true;
        return;
    }

    n_printf("Warning: Invalid CVar type passed to CVarWrite.\n");
    return;
}

//------------------------------------------------------------------------------
/**
*/
int const
CVarReadInt(CVar* cVar)
{
    if (cVar->value.type == CVar_Int)
    {
        return cVar->value.i;
    }

    n_printf("Warning: Trying to read incorrect type from CVar.\n");
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
float const
CVarReadFloat(CVar* cVar)
{
    if (cVar->value.type == CVar_Float)
    {
        return cVar->value.f;
    }

    n_printf("Warning: Trying to read incorrect type from CVar.\n");
    return 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
const char*
CVarReadString(CVar* cVar)
{
    if (cVar->value.type == CVar_String)
    {
        return cVar->value.cstr;
    }

    n_printf("Warning: Trying to read incorrect type from CVar.\n");
    return nullptr;
}

//------------------------------------------------------------------------------
/**
*/
bool
CVarModified(CVar* cVar)
{
    return cVar->modified;
}

//------------------------------------------------------------------------------
/**
*/
void
CVarSetModified(CVar* cVar, bool value)
{
    cVar->modified = value;
}

//------------------------------------------------------------------------------
/**
*/
CVarType
CVarGetType(CVar* cVar)
{
    return cVar->value.type;
}

//------------------------------------------------------------------------------
/**
*/
const char*
CVarGetName(CVar* cVar)
{
    return cVar->name.AsCharPtr();
}

//------------------------------------------------------------------------------
/**
*/
const char*
CVarGetDescription(CVar* cVar)
{
    return cVar->description.AsCharPtr();
}

//------------------------------------------------------------------------------
/**
*/
int
CVarNum()
{
    return cVarOffset;
}

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarsBegin()
{
    return cVars;
}

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarsEnd()
{
    return cVars + cVarOffset;
}

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarNext(CVar* cVar)
{
    return cVar + 1;
}


} // namespace Core
