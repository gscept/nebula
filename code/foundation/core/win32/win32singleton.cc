//------------------------------------------------------------------------------
//  win32singleton.cc
//  (C) 2026 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "core/win32/win32singleton.h"

#include "util/hashtable.h"
#include "util/string.h"
#include "threading/criticalsection.h"
#include "system/moduleinterface.h"

namespace Core
{
namespace
{
using SingletonTable = Util::HashTable<Util::String, void*>;

//------------------------------------------------------------------------------
/**
*/
static SingletonTable&
GetGlobalSingletonTable()
{
    static SingletonTable table;
    return table;
}

//------------------------------------------------------------------------------
/**
*/
static SingletonTable&
GetThreadLocalSingletonTable()
{
    thread_local SingletonTable table;
    return table;
}

//------------------------------------------------------------------------------
/**
*/
static Threading::CriticalSection&
GetGlobalSingletonLock()
{
    static Threading::CriticalSection criticalSection;
    return criticalSection;
}

//------------------------------------------------------------------------------
/**
*/
static void*
LocalSingletonGet(const char* key, bool threadLocal)
{
    SingletonTable& table = threadLocal ? GetThreadLocalSingletonTable() : GetGlobalSingletonTable();
    const Util::String lookupKey = key;

    if (!threadLocal)
        GetGlobalSingletonLock().Enter();

    void* ptr = nullptr;
    IndexT index = table.FindIndex(lookupKey);
    if (index != InvalidIndex)
        ptr = table.ValueAtIndex(lookupKey, index);

    if (!threadLocal)
        GetGlobalSingletonLock().Leave();

    return ptr;
}

//------------------------------------------------------------------------------
/**
*/
static void
LocalSingletonSet(const char* key, bool threadLocal, void* ptr)
{
    SingletonTable& table = threadLocal ? GetThreadLocalSingletonTable() : GetGlobalSingletonTable();
    const Util::String lookupKey = key;

    if (!threadLocal)
        GetGlobalSingletonLock().Enter();

    IndexT index = table.FindIndex(lookupKey);
    if (index == InvalidIndex)
    {
        table.Add(lookupKey, ptr);
    }
    else
    {
        table.ValueAtIndex(lookupKey, index) = ptr;
    }

    if (!threadLocal)
        GetGlobalSingletonLock().Leave();
}

NEBULA_MODULE_EXPORT void* NebulaHost_SingletonGet(const char* key, bool threadLocal);
NEBULA_MODULE_EXPORT void NebulaHost_SingletonSet(const char* key, bool threadLocal, void* ptr);

struct HostSingletonApi
{
    void* (*get)(const char*, bool) = nullptr;
    void (*set)(const char*, bool, void*) = nullptr;
    bool resolved = false;
    bool valid = false;
};

//------------------------------------------------------------------------------
/**
*/
static HostSingletonApi&
GetHostSingletonApi()
{
    static HostSingletonApi api;
    if (!api.resolved)
    {
        api.resolved = true;
        HMODULE exe = ::GetModuleHandleA(nullptr);
        if (exe != nullptr)
        {
            api.get = reinterpret_cast<void* (*)(const char*, bool)>(::GetProcAddress(exe, "NebulaHost_SingletonGet"));
            api.set = reinterpret_cast<void (*)(const char*, bool, void*)>(::GetProcAddress(exe, "NebulaHost_SingletonSet"));
            api.valid = api.get != nullptr && api.set != nullptr;
        }
    }
    return api;
}

//------------------------------------------------------------------------------
/**
*/
static bool
UseHostSingletonApi()
{
    HostSingletonApi& api = GetHostSingletonApi();
    return api.valid && api.get != &NebulaHost_SingletonGet && api.set != &NebulaHost_SingletonSet;
}
} // anonymous namespace

//------------------------------------------------------------------------------
/**
*/
void*
SingletonGet(const char* key, bool threadLocal)
{
    if (UseHostSingletonApi())
    {
        HostSingletonApi& api = GetHostSingletonApi();
        return api.get(key, threadLocal);
    }
    return LocalSingletonGet(key, threadLocal);
}

//------------------------------------------------------------------------------
/**
*/
void
SingletonSet(const char* key, bool threadLocal, void* ptr)
{
    if (UseHostSingletonApi())
    {
        HostSingletonApi& api = GetHostSingletonApi();
        api.set(key, threadLocal, ptr);
        return;
    }
    LocalSingletonSet(key, threadLocal, ptr);
}

//------------------------------------------------------------------------------
/**
*/
NEBULA_MODULE_EXPORT void*
NebulaHost_SingletonGet(const char* key, bool threadLocal)
{
    return LocalSingletonGet(key, threadLocal);
}

//------------------------------------------------------------------------------
/**
*/
NEBULA_MODULE_EXPORT void
NebulaHost_SingletonSet(const char* key, bool threadLocal, void* ptr)
{
    LocalSingletonSet(key, threadLocal, ptr);
}

} // namespace Core
