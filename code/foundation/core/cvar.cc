//------------------------------------------------------------------------------
//  cvar.cc
//  @copyright (C) 2021$ Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
//
#include "cvar.h"
#include "system/moduleinterface.h"
#include "util/hashtable.h"
#include "util/string.h"

#if __WIN32__
#include <windows.h>
#endif

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

static Util::HashTable<Util::String, uint16_t>&
GetLocalCVarTable()
{
    static Util::HashTable<Util::String, uint16_t> table;
    return table;
}

static CVar* CVarCreateLocal(CVarCreateInfo const& info);
static CVar* CVarGetLocal(const char* name);
static void CVarParseWriteLocal(CVar* cVar, const char* value);
static void CVarWriteFloatLocal(CVar* cVar, float value);
static void CVarWriteIntLocal(CVar* cVar, int value);
static void CVarWriteStringLocal(CVar* cVar, const char* value);
static int CVarReadIntLocal(CVar* cVar);
static float CVarReadFloatLocal(CVar* cVar);
static const char* CVarReadStringLocal(CVar* cVar);
static bool CVarModifiedLocal(CVar* cVar);
static void CVarSetModifiedLocal(CVar* cVar, bool value);
static CVarType CVarGetTypeLocal(CVar* cVar);
static const char* CVarGetNameLocal(CVar* cVar);
static const char* CVarGetDescriptionLocal(CVar* cVar);
static int CVarNumLocal();
static CVar* CVarsBeginLocal();
static CVar* CVarsEndLocal();
static CVar* CVarNextLocal(CVar* cVar);

#if __WIN32__
NEBULA_MODULE_EXPORT CVar* NebulaHost_CVarCreate(int type, const char* name, const char* defaultValue, const char* description);
NEBULA_MODULE_EXPORT CVar* NebulaHost_CVarGet(const char* name);
NEBULA_MODULE_EXPORT void NebulaHost_CVarParseWrite(CVar* cVar, const char* value);
NEBULA_MODULE_EXPORT void NebulaHost_CVarWriteFloat(CVar* cVar, float value);
NEBULA_MODULE_EXPORT void NebulaHost_CVarWriteInt(CVar* cVar, int value);
NEBULA_MODULE_EXPORT void NebulaHost_CVarWriteString(CVar* cVar, const char* value);
NEBULA_MODULE_EXPORT int NebulaHost_CVarReadInt(CVar* cVar);
NEBULA_MODULE_EXPORT float NebulaHost_CVarReadFloat(CVar* cVar);
NEBULA_MODULE_EXPORT const char* NebulaHost_CVarReadString(CVar* cVar);
NEBULA_MODULE_EXPORT bool NebulaHost_CVarModified(CVar* cVar);
NEBULA_MODULE_EXPORT void NebulaHost_CVarSetModified(CVar* cVar, bool value);
NEBULA_MODULE_EXPORT CVarType NebulaHost_CVarGetType(CVar* cVar);
NEBULA_MODULE_EXPORT const char* NebulaHost_CVarGetName(CVar* cVar);
NEBULA_MODULE_EXPORT const char* NebulaHost_CVarGetDescription(CVar* cVar);
NEBULA_MODULE_EXPORT int NebulaHost_CVarNum();
NEBULA_MODULE_EXPORT CVar* NebulaHost_CVarsBegin();
NEBULA_MODULE_EXPORT CVar* NebulaHost_CVarsEnd();
NEBULA_MODULE_EXPORT CVar* NebulaHost_CVarNext(CVar* cVar);

struct HostCVarApi
{
    CVar* (*create)(int, const char*, const char*, const char*) = nullptr;
    CVar* (*get)(const char*) = nullptr;
    void (*parseWrite)(CVar*, const char*) = nullptr;
    void (*writeFloat)(CVar*, float) = nullptr;
    void (*writeInt)(CVar*, int) = nullptr;
    void (*writeString)(CVar*, const char*) = nullptr;
    int (*readInt)(CVar*) = nullptr;
    float (*readFloat)(CVar*) = nullptr;
    const char* (*readString)(CVar*) = nullptr;
    bool (*modified)(CVar*) = nullptr;
    void (*setModified)(CVar*, bool) = nullptr;
    CVarType (*getType)(CVar*) = nullptr;
    const char* (*getName)(CVar*) = nullptr;
    const char* (*getDescription)(CVar*) = nullptr;
    int (*num)() = nullptr;
    CVar* (*begin)() = nullptr;
    CVar* (*end)() = nullptr;
    CVar* (*next)(CVar*) = nullptr;
    bool resolved = false;
    bool valid = false;
};

static HostCVarApi&
GetHostCVarApi()
{
    static HostCVarApi api;
    if (!api.resolved)
    {
        api.resolved = true;
        HMODULE exe = ::GetModuleHandleA(nullptr);
        if (exe != nullptr)
        {
            api.create = reinterpret_cast<CVar* (*)(int, const char*, const char*, const char*)>(::GetProcAddress(exe, "NebulaHost_CVarCreate"));
            api.get = reinterpret_cast<CVar* (*)(const char*)>(::GetProcAddress(exe, "NebulaHost_CVarGet"));
            api.parseWrite = reinterpret_cast<void (*)(CVar*, const char*)>(::GetProcAddress(exe, "NebulaHost_CVarParseWrite"));
            api.writeFloat = reinterpret_cast<void (*)(CVar*, float)>(::GetProcAddress(exe, "NebulaHost_CVarWriteFloat"));
            api.writeInt = reinterpret_cast<void (*)(CVar*, int)>(::GetProcAddress(exe, "NebulaHost_CVarWriteInt"));
            api.writeString = reinterpret_cast<void (*)(CVar*, const char*)>(::GetProcAddress(exe, "NebulaHost_CVarWriteString"));
            api.readInt = reinterpret_cast<int (*)(CVar*)>(::GetProcAddress(exe, "NebulaHost_CVarReadInt"));
            api.readFloat = reinterpret_cast<float (*)(CVar*)>(::GetProcAddress(exe, "NebulaHost_CVarReadFloat"));
            api.readString = reinterpret_cast<const char* (*)(CVar*)>(::GetProcAddress(exe, "NebulaHost_CVarReadString"));
            api.modified = reinterpret_cast<bool (*)(CVar*)>(::GetProcAddress(exe, "NebulaHost_CVarModified"));
            api.setModified = reinterpret_cast<void (*)(CVar*, bool)>(::GetProcAddress(exe, "NebulaHost_CVarSetModified"));
            api.getType = reinterpret_cast<CVarType (*)(CVar*)>(::GetProcAddress(exe, "NebulaHost_CVarGetType"));
            api.getName = reinterpret_cast<const char* (*)(CVar*)>(::GetProcAddress(exe, "NebulaHost_CVarGetName"));
            api.getDescription = reinterpret_cast<const char* (*)(CVar*)>(::GetProcAddress(exe, "NebulaHost_CVarGetDescription"));
            api.num = reinterpret_cast<int (*)()>(::GetProcAddress(exe, "NebulaHost_CVarNum"));
            api.begin = reinterpret_cast<CVar* (*)()>(::GetProcAddress(exe, "NebulaHost_CVarsBegin"));
            api.end = reinterpret_cast<CVar* (*)()>(::GetProcAddress(exe, "NebulaHost_CVarsEnd"));
            api.next = reinterpret_cast<CVar* (*)(CVar*)>(::GetProcAddress(exe, "NebulaHost_CVarNext"));

            api.valid =
                api.create != nullptr &&
                api.get != nullptr &&
                api.parseWrite != nullptr &&
                api.writeFloat != nullptr &&
                api.writeInt != nullptr &&
                api.writeString != nullptr &&
                api.readInt != nullptr &&
                api.readFloat != nullptr &&
                api.readString != nullptr &&
                api.modified != nullptr &&
                api.setModified != nullptr &&
                api.getType != nullptr &&
                api.getName != nullptr &&
                api.getDescription != nullptr &&
                api.num != nullptr &&
                api.begin != nullptr &&
                api.end != nullptr &&
                api.next != nullptr;
        }
    }
    return api;
}

static bool
UseHostCVarApi()
{
    HostCVarApi& api = GetHostCVarApi();
    return api.valid && api.create != &NebulaHost_CVarCreate;
}

#define CVAR_FORWARD_TO_HOST_RET(member, ...) \
    do { \
        if (UseHostCVarApi()) \
        { \
            HostCVarApi& cvarHostApi = GetHostCVarApi(); \
            return cvarHostApi.member(__VA_ARGS__); \
        } \
    } while (false)

#define CVAR_FORWARD_TO_HOST_VOID(member, ...) \
    do { \
        if (UseHostCVarApi()) \
        { \
            HostCVarApi& cvarHostApi = GetHostCVarApi(); \
            cvarHostApi.member(__VA_ARGS__); \
            return; \
        } \
    } while (false)

#else

#define CVAR_FORWARD_TO_HOST_RET(member, ...) do { } while (false)
#define CVAR_FORWARD_TO_HOST_VOID(member, ...) do { } while (false)

#endif

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarCreate(CVarCreateInfo const& info)
{
    CVAR_FORWARD_TO_HOST_RET(create, (int)info.type, info.name, info.defaultValue, info.description);
    return CVarCreateLocal(info);
}

//------------------------------------------------------------------------------
/**
*/
static CVar*
CVarCreateLocal(CVarCreateInfo const& info)
{
    CVar* ptr = CVarGetLocal(info.name);
    const bool needsInit = (ptr == nullptr);
    if (ptr == nullptr)
    {
        IndexT varIndex = cVarOffset++;
        n_assert(varIndex < MAX_CVARS);
        ptr = &cVars[varIndex];
        GetLocalCVarTable().Add(info.name, varIndex);
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
        CVarParseWriteLocal(ptr, info.defaultValue);
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
    CVAR_FORWARD_TO_HOST_RET(get, name);
    return CVarGetLocal(name);
}

//------------------------------------------------------------------------------
/**
*/
static CVar*
CVarGetLocal(const char* name)
{
    Util::String const str = name;
    Util::HashTable<Util::String, uint16_t>& cVarTable = GetLocalCVarTable();
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
    CVAR_FORWARD_TO_HOST_VOID(parseWrite, cVar, value);
    CVarParseWriteLocal(cVar, value);
}

//------------------------------------------------------------------------------
/**
*/
static void
CVarParseWriteLocal(CVar* cVar, const char* value)
{
    switch (cVar->value.type)
    {
    case CVar_Int:
        CVarWriteIntLocal(cVar, atoi(value));
        break;
    case CVar_Float:
        CVarWriteFloatLocal(cVar, atof(value));
        break;
    case CVar_String:
        CVarWriteStringLocal(cVar, value);
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
    CVAR_FORWARD_TO_HOST_VOID(writeFloat, cVar, value);
    CVarWriteFloatLocal(cVar, value);
}

//------------------------------------------------------------------------------
/**
*/
static void
CVarWriteFloatLocal(CVar* cVar, float value)
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
    CVAR_FORWARD_TO_HOST_VOID(writeInt, cVar, value);
    CVarWriteIntLocal(cVar, value);
}

//------------------------------------------------------------------------------
/**
*/
static void
CVarWriteIntLocal(CVar* cVar, int value)
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
    CVAR_FORWARD_TO_HOST_VOID(writeString, cVar, value);
    CVarWriteStringLocal(cVar, value);
}

//------------------------------------------------------------------------------
/**
*/
static void
CVarWriteStringLocal(CVar* cVar, const char* value)
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
    CVAR_FORWARD_TO_HOST_RET(readInt, cVar);
    return CVarReadIntLocal(cVar);
}

//------------------------------------------------------------------------------
/**
*/
static int
CVarReadIntLocal(CVar* cVar)
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
    CVAR_FORWARD_TO_HOST_RET(readFloat, cVar);
    return CVarReadFloatLocal(cVar);
}

//------------------------------------------------------------------------------
/**
*/
static float
CVarReadFloatLocal(CVar* cVar)
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
    CVAR_FORWARD_TO_HOST_RET(readString, cVar);
    return CVarReadStringLocal(cVar);
}

//------------------------------------------------------------------------------
/**
*/
static const char*
CVarReadStringLocal(CVar* cVar)
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
    CVAR_FORWARD_TO_HOST_RET(modified, cVar);
    return CVarModifiedLocal(cVar);
}

//------------------------------------------------------------------------------
/**
*/
static bool
CVarModifiedLocal(CVar* cVar)
{
    return cVar->modified;
}

//------------------------------------------------------------------------------
/**
*/
void
CVarSetModified(CVar* cVar, bool value)
{
    CVAR_FORWARD_TO_HOST_VOID(setModified, cVar, value);
    CVarSetModifiedLocal(cVar, value);
}

//------------------------------------------------------------------------------
/**
*/
static void
CVarSetModifiedLocal(CVar* cVar, bool value)
{
    cVar->modified = value;
}

//------------------------------------------------------------------------------
/**
*/
CVarType
CVarGetType(CVar* cVar)
{
    CVAR_FORWARD_TO_HOST_RET(getType, cVar);
    return CVarGetTypeLocal(cVar);
}

//------------------------------------------------------------------------------
/**
*/
static CVarType
CVarGetTypeLocal(CVar* cVar)
{
    return cVar->value.type;
}

//------------------------------------------------------------------------------
/**
*/
const char*
CVarGetName(CVar* cVar)
{
    CVAR_FORWARD_TO_HOST_RET(getName, cVar);
    return CVarGetNameLocal(cVar);
}

//------------------------------------------------------------------------------
/**
*/
static const char*
CVarGetNameLocal(CVar* cVar)
{
    return cVar->name.AsCharPtr();
}

//------------------------------------------------------------------------------
/**
*/
const char*
CVarGetDescription(CVar* cVar)
{
    CVAR_FORWARD_TO_HOST_RET(getDescription, cVar);
    return CVarGetDescriptionLocal(cVar);
}

//------------------------------------------------------------------------------
/**
*/
static const char*
CVarGetDescriptionLocal(CVar* cVar)
{
    return cVar->description.AsCharPtr();
}

//------------------------------------------------------------------------------
/**
*/
int
CVarNum()
{
    CVAR_FORWARD_TO_HOST_RET(num);
    return CVarNumLocal();
}

//------------------------------------------------------------------------------
/**
*/
static int
CVarNumLocal()
{
    return cVarOffset;
}

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarsBegin()
{
    CVAR_FORWARD_TO_HOST_RET(begin);
    return CVarsBeginLocal();
}

//------------------------------------------------------------------------------
/**
*/
static CVar*
CVarsBeginLocal()
{
    return cVars;
}

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarsEnd()
{
    CVAR_FORWARD_TO_HOST_RET(end);
    return CVarsEndLocal();
}

//------------------------------------------------------------------------------
/**
*/
static CVar*
CVarsEndLocal()
{
    return cVars + cVarOffset;
}

//------------------------------------------------------------------------------
/**
*/
CVar*
CVarNext(CVar* cVar)
{
    CVAR_FORWARD_TO_HOST_RET(next, cVar);
    return CVarNextLocal(cVar);
}

//------------------------------------------------------------------------------
/**
*/
static CVar*
CVarNextLocal(CVar* cVar)
{
    return cVar + 1;
}

#if __WIN32__
NEBULA_MODULE_EXPORT CVar*
NebulaHost_CVarCreate(int type, const char* name, const char* defaultValue, const char* description)
{
    CVarCreateInfo info;
    info.type = (CVarType)type;
    info.name = name;
    info.defaultValue = defaultValue;
    info.description = description;
    return CVarCreateLocal(info);
}

NEBULA_MODULE_EXPORT CVar*
NebulaHost_CVarGet(const char* name)
{
    return CVarGetLocal(name);
}

NEBULA_MODULE_EXPORT void
NebulaHost_CVarParseWrite(CVar* cVar, const char* value)
{
    CVarParseWriteLocal(cVar, value);
}

NEBULA_MODULE_EXPORT void
NebulaHost_CVarWriteFloat(CVar* cVar, float value)
{
    CVarWriteFloatLocal(cVar, value);
}

NEBULA_MODULE_EXPORT void
NebulaHost_CVarWriteInt(CVar* cVar, int value)
{
    CVarWriteIntLocal(cVar, value);
}

NEBULA_MODULE_EXPORT void
NebulaHost_CVarWriteString(CVar* cVar, const char* value)
{
    CVarWriteStringLocal(cVar, value);
}

NEBULA_MODULE_EXPORT int
NebulaHost_CVarReadInt(CVar* cVar)
{
    return CVarReadIntLocal(cVar);
}

NEBULA_MODULE_EXPORT float
NebulaHost_CVarReadFloat(CVar* cVar)
{
    return CVarReadFloatLocal(cVar);
}

NEBULA_MODULE_EXPORT const char*
NebulaHost_CVarReadString(CVar* cVar)
{
    return CVarReadStringLocal(cVar);
}

NEBULA_MODULE_EXPORT bool
NebulaHost_CVarModified(CVar* cVar)
{
    return CVarModifiedLocal(cVar);
}

NEBULA_MODULE_EXPORT void
NebulaHost_CVarSetModified(CVar* cVar, bool value)
{
    CVarSetModifiedLocal(cVar, value);
}

NEBULA_MODULE_EXPORT CVarType
NebulaHost_CVarGetType(CVar* cVar)
{
    return CVarGetTypeLocal(cVar);
}

NEBULA_MODULE_EXPORT const char*
NebulaHost_CVarGetName(CVar* cVar)
{
    return CVarGetNameLocal(cVar);
}

NEBULA_MODULE_EXPORT const char*
NebulaHost_CVarGetDescription(CVar* cVar)
{
    return CVarGetDescriptionLocal(cVar);
}

NEBULA_MODULE_EXPORT int
NebulaHost_CVarNum()
{
    return CVarNumLocal();
}

NEBULA_MODULE_EXPORT CVar*
NebulaHost_CVarsBegin()
{
    return CVarsBeginLocal();
}

NEBULA_MODULE_EXPORT CVar*
NebulaHost_CVarsEnd()
{
    return CVarsEndLocal();
}

NEBULA_MODULE_EXPORT CVar*
NebulaHost_CVarNext(CVar* cVar)
{
    return CVarNextLocal(cVar);
}
#endif


} // namespace Core
