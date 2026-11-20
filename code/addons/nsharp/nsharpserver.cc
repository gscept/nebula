//------------------------------------------------------------------------------
//  nsharpserver.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "nsharpserver.h"
#include "io/ioserver.h"
#include "io/textreader.h"
#include "debug/debugserver.h"
#include "nsbindings.h"

#ifdef _WIN32
#include "util/win32/win32stringconverter.h"
#endif

// .net apphost includes
#include "nethost.h"
#include "coreclr_delegates.h"
#include "hostfxr.h"

using namespace IO;

namespace Scripting
{
__ImplementClass(Scripting::NSharpServer, 'N#Sv', Core::RefCounted);
__ImplementSingleton(Scripting::NSharpServer);

using namespace Util;
using namespace IO;

enum HostFxrStatusCode
{
    // Success
    Success = 0,
    Success_HostAlreadyInitialized = 0x00000001,
    Success_DifferentRuntimeProperties = 0x00000002,
};

struct NSharpServer::Assembly
{
    Util::String name;    // assembly name
    Util::String dllPath; // path to dll
    load_assembly_and_get_function_pointer_fn GetExport;
    Util::Dictionary<Util::String, void*> entryPoints;
};

struct DotNET_API
{
    // clang-format off
    hostfxr_initialize_for_runtime_config_fn InitConfig             = nullptr;
    hostfxr_get_runtime_delegate_fn GetDelegate                     = nullptr;
    hostfxr_close_fn Close                                          = nullptr;
    // clang-format on

    // validate that the api is correctly loaded.
    bool
    Validate()
    {
        return InitConfig && GetDelegate && Close;
    }
};

static DotNET_API api;

//------------------------------------------------------------------------------
/**
    nethost and hostfxr take char_t strings: wchar_t (UTF-16) on Windows, char (UTF-8) on Linux.
    Util::String is UTF-8, so this converts on Windows and copies on Linux.
    out must be at least NEBULA_MAXPATH char_t wide.
*/
static void
ToDotNetString(Util::String const& str, char_t* out)
{
#ifdef _WIN32
    Win32::Win32StringConverter::UTF8ToWide(str, (ushort*)out, NEBULA_MAXPATH * sizeof(char_t));
#else
    const char* src = str.AsCharPtr();
    size_t i = 0;
    while (src[i] != 0)
    {
        out[i] = (char_t)src[i];
        i++;
    }
    out[i] = 0;
#endif
}

//------------------------------------------------------------------------------
/**
*/
NSharpServer::NSharpServer()
    : isOpen(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
NSharpServer::~NSharpServer()
{
    if (this->IsOpen())
    {
        this->Close();
    }
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
    Using the nethost library, discover the location of hostfxr and get exports
*/
bool
NSharpServer::LoadHostFxr()
{
    // Pre-allocate a large buffer for the path to hostfxr
    char_t buffer[NEBULA_MAXPATH];
    size_t buffer_size = sizeof(buffer) / sizeof(char_t);
    int rc = get_hostfxr_path(buffer, &buffer_size, nullptr);
    if (rc != 0)
        return false;

#ifdef _WIN32
    Util::String path = Win32::Win32StringConverter::WideToUTF8((const ushort*)buffer);
#else
    Util::String path(buffer);
#endif

    // Load hostfxr and get desired exports
    this->hostfxr.SetPath(IO::URI(path));
    if (!this->hostfxr.Load())
        return false;

    // clang-format off
    api = {
        .InitConfig=(hostfxr_initialize_for_runtime_config_fn)this->hostfxr.GetExport("hostfxr_initialize_for_runtime_config"),
        .GetDelegate=(hostfxr_get_runtime_delegate_fn)this->hostfxr.GetExport("hostfxr_get_runtime_delegate"),
        .Close=(hostfxr_close_fn)this->hostfxr.GetExport("hostfxr_close")
    };
    // clang-format on

    return api.Validate();
}

//------------------------------------------------------------------------------
/**
*/
void
NSharpServer::CloseHostFxr()
{
    this->hostfxr.Close();
    api = {};
}

//------------------------------------------------------------------------------
/**
    Function should be formatted as: "Namespace.Namespace.Class+NestedClass::Function()"
*/
void*
NSharpServer::GetDelegatePointer(AssemblyId assemblyId, Util::String const& function, Util::String const& delegateName)
{
    if (assemblyId.id >= this->assemblies.Size() || assemblyId == InvalidIndex)
    {
        n_warning("Invalid assembly id!");
        return nullptr;
    }

    Assembly* assembly = this->assemblies.Get<0>(assemblyId.id);

    // Separate class and namespace from string.
    auto methodSeparatorIndex = function.FindCharIndex(':');
    Util::String ns = function.ExtractRange(0, methodSeparatorIndex);
    ns += ", ";
    ns += assembly->name;
    Util::String methodName = function.ExtractRange(methodSeparatorIndex + 2, function.Length() - methodSeparatorIndex - 4);

    char_t pathBuffer[NEBULA_MAXPATH];
    char_t typeBuffer[NEBULA_MAXPATH];
    char_t methodBuffer[NEBULA_MAXPATH];
    char_t delegateBuffer[NEBULA_MAXPATH];
    ToDotNetString(assembly->dllPath, pathBuffer);
    ToDotNetString(ns, typeBuffer);
    ToDotNetString(methodName, methodBuffer);
    ToDotNetString(delegateName, delegateBuffer);

    void* func = nullptr;

    int rc = assembly->GetExport(
        pathBuffer,
        typeBuffer,
        methodBuffer,
        delegateBuffer,
        nullptr,
        &func
    );

    if (rc != 0 || func == nullptr)
    {
        n_warning("Could not find function delegate '%s' in assembly (%s)!", function.AsCharPtr(), assembly->name.AsCharPtr());
        n_printf("\tPossible solutions:\n"
                 "\t\tFunction name is correctly formatted (ex. Namespace.Namespace.Class+NestedClass::Function())?\n"
                 "\t\tDelegate type name correctly formatted?");
        return nullptr;
    }

    return func;
}

//------------------------------------------------------------------------------
/**
    Function should be formatted as: "Namespace.Namespace.Class+NestedClass::Function()"
    Resolved pointers are cached on the assembly so that the frame entry points do not have to be
    looked up through hostfxr every frame. A cached pointer is only valid while the assembly it
    came from stays loaded; the engine assembly is never unloaded, so its entry points survive a
    plugin reload.
*/
void*
NSharpServer::GetUnmanagedFuncPointer(AssemblyId assemblyId, Util::String const& function)
{
    if (assemblyId.id >= this->assemblies.Size() || assemblyId == InvalidIndex)
    {
        n_warning("Invalid assembly id!");
        return nullptr;
    }

    Assembly* assembly = this->assemblies.Get<0>(assemblyId.id);

    IndexT cachedIndex = assembly->entryPoints.FindIndex(function);
    if (cachedIndex != InvalidIndex)
        return assembly->entryPoints.ValueAtIndex(cachedIndex);

    // Separate class and namespace from string.
    auto methodSeparatorIndex = function.FindCharIndex(':');
    Util::String ns = function.ExtractRange(0, methodSeparatorIndex);
    ns += ", ";
    ns += assembly->name;
    Util::String methodName = function.ExtractRange(methodSeparatorIndex + 2, function.Length() - methodSeparatorIndex - 4);

    char_t pathBuffer[NEBULA_MAXPATH];
    char_t typeBuffer[NEBULA_MAXPATH];
    char_t methodBuffer[NEBULA_MAXPATH];
    ToDotNetString(assembly->dllPath, pathBuffer);
    ToDotNetString(ns, typeBuffer);
    ToDotNetString(methodName, methodBuffer);

    void* func = nullptr;

    int rc = assembly->GetExport(
        pathBuffer,
        typeBuffer,
        methodBuffer,
        UNMANAGEDCALLERSONLY_METHOD,
        nullptr,
        &func
    );

    if (rc != 0 || func == nullptr)
    {
        n_warning("Could not find function '%s' in assembly (%s)!", function.AsCharPtr(), assembly->name.AsCharPtr());
        n_printf("\tPossible solutions:\n"
                 "\t\tFunction name is correctly formatted (ex. Namespace.Namespace.Class+NestedClass::Function())?\n"
                 "\t\tFunction is declared with the [UnmanagedCallersOnly] attribute in C#?");
        return nullptr;
    }

    assembly->entryPoints.Add(function, func);
    return func;
}

//------------------------------------------------------------------------------
/**
*/
load_assembly_and_get_function_pointer_fn
LoadAssemblyAndGetExport(Util::String const& configPath)
{
    // Load .NET Core
    void* load_assembly_and_get_function_pointer = nullptr;
    hostfxr_handle cxt = nullptr;

    char_t configBuffer[NEBULA_MAXPATH];
    ToDotNetString(configPath, configBuffer);

    int rc = api.InitConfig(configBuffer, nullptr, &cxt);
    if (((rc != Success && rc != Success_HostAlreadyInitialized && rc != Success_DifferentRuntimeProperties) || cxt == nullptr))
    {
        n_error("Failed to initialize .NET Core!");
        api.Close(cxt);
    }

    // Get the load assembly function pointer
    rc = api.GetDelegate(cxt, hdt_load_assembly_and_get_function_pointer, &load_assembly_and_get_function_pointer);
    if (rc != 0 || load_assembly_and_get_function_pointer == nullptr)
    {
        n_error("Failed to get delegate when loading dotnet assembly!");
    }

    api.Close(cxt);
    return reinterpret_cast<load_assembly_and_get_function_pointer_fn>(load_assembly_and_get_function_pointer);
}

//------------------------------------------------------------------------------
/**
*/
bool
NSharpServer::Open()
{
    n_assert(!this->IsOpen());
    // Host custom .net runtime
    this->isOpen = this->LoadHostFxr();
    n_assert(this->isOpen);

    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
void
NSharpServer::Close()
{
    n_assert(this->IsOpen());

    // The CLR cannot be unloaded, but the assemblies we registered with it can be dropped.
    // Cached entry points go with them.
    for (uint32_t i = 0; i < this->assemblies.Size(); i++)
    {
        Assembly* assembly = this->assemblies.Get<0>(i);
        if (assembly != nullptr)
            delete assembly;
    }
    this->assemblies.EraseRange(0, this->assemblies.Size());

    this->CloseHostFxr();

    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
AssemblyId
NSharpServer::LoadAssembly(IO::URI const& uri)
{
    Util::String configPath = uri.GetHostAndLocalPath();
    configPath.StripFileExtension();
    configPath += ".runtimeconfig.json";

    Assembly* assembly = new Assembly();
    assembly->GetExport = (load_assembly_and_get_function_pointer_fn)LoadAssemblyAndGetExport(configPath);

    if (assembly->GetExport == nullptr)
    {
        n_warning("Could not load .NET assembly!");
        delete assembly;
        return InvalidIndex;
    }

    assembly->dllPath = uri.GetHostAndLocalPath();
    assembly->name = uri.AsString().ExtractFileName();
    assembly->name.StripFileExtension();

    AssemblyId assemblyId = this->assemblies.Alloc();
    this->assemblies.Get<0>(assemblyId.id) = assembly;
    return assemblyId;
}

//------------------------------------------------------------------------------
/**
	Function should be formatted as: "Namespace.Namespace.Class+NestedClass::Function()"
*/
int
NSharpServer::ExecUnmanagedCall(AssemblyId assemblyId, Util::String const& function)
{
    typedef void(CORECLR_DELEGATE_CALLTYPE * custom_entry_point_fn)();
    custom_entry_point_fn func = nullptr;
    func = (custom_entry_point_fn)GetUnmanagedFuncPointer(assemblyId, function);
    if (func != nullptr)
    {
        func();
        return 0;
    }

    n_printf("[NSharpServer] Could not find managed function '%s'!\n", function.AsCharPtr());
    return 1;
}

//------------------------------------------------------------------------------
/**
	Function should be formatted as: "Namespace.Namespace.Class+NestedClass::Function()"
*/
int
NSharpServer::ExecUnmanagedCall(
    AssemblyId assemblyId,
    Util::String const& function,
    Util::String const& argument
)
{
    typedef int(CORECLR_DELEGATE_CALLTYPE * custom_entry_point_fn)(const char*, int32_t);
    custom_entry_point_fn func = nullptr;
    func = (custom_entry_point_fn)GetUnmanagedFuncPointer(assemblyId, function);
    if (func != nullptr)
    {
        return func(argument.AsCharPtr(), argument.Length());
    }

    n_printf("[NSharpServer] Could not find managed function '%s'!\n", function.AsCharPtr());
    return 1;
}

//------------------------------------------------------------------------------
/**
*/
bool const
NSharpServer::IsOpen()
{
    return this->isOpen;
}

} // namespace Scripting
