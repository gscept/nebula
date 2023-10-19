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
#include "util/commandlineargs.h"

#include <string>
#include <locale>
#include <codecvt>

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
    std::wstring dllPath; // path to dll
    load_assembly_and_get_function_pointer_fn GetExport;
};

struct DotNET_API
{
    // clang-format off
    hostfxr_initialize_for_runtime_config_fn InitConfig             = nullptr;
    hostfxr_initialize_for_dotnet_command_line_fn InitCommandLine   = nullptr;
    hostfxr_get_runtime_delegate_fn GetDelegate                     = nullptr;
    hostfxr_close_fn Close                                          = nullptr;
    // clang-format on

    // validate that the api is correctly loaded.
    bool
    Validate()
    {
        return InitConfig && InitCommandLine && GetDelegate && Close;
    }
};

static DotNET_API api;

//------------------------------------------------------------------------------
/**
*/
NSharpServer::NSharpServer()
    : isOpen(false),
      waitForDebugger(false),
      debuggerEnabled(false)
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
*/
static void*
ScriptingAlloc(size_t bytes)
{
    return Memory::Alloc(Memory::HeapType::ScriptingHeap, bytes);
}

//------------------------------------------------------------------------------
/**
*/
static void*
ScriptingRealloc(void* ptr, size_t bytes)
{
    return Memory::Realloc(Memory::HeapType::ScriptingHeap, ptr, bytes);
}

//------------------------------------------------------------------------------
/**
*/
static void
ScriptingDealloc(void* ptr)
{
    Memory::Free(Memory::HeapType::ScriptingHeap, ptr);
}

//------------------------------------------------------------------------------
/**
*/
static void*
ScriptingCalloc(size_t count, size_t size)
{
    const size_t bytes = size * count;
    void* ptr = Memory::Alloc(Memory::HeapType::ScriptingHeap, bytes);
    memset(ptr, 0, bytes);
    return ptr;
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

    char path[NEBULA_MAXPATH];
    // Convert wide char path to const char*
    sprintf(path, "%ws", buffer);

    // Load hostfxr and get desired exports
    this->hostfxr.SetPath(path);
    if (!this->hostfxr.Load())
        return false;

    // clang-format off
    api = {
        .InitConfig=(hostfxr_initialize_for_runtime_config_fn)this->hostfxr.GetExport("hostfxr_initialize_for_runtime_config"),
        .InitCommandLine=(hostfxr_initialize_for_dotnet_command_line_fn)this->hostfxr.GetExport("hostfxr_initialize_for_dotnet_command_line"),
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
*/
std::wstring
ToWideString(Util::String const& str)
{
#ifdef _WIN32
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t> convert;
#else
#error "Not yet implemented on this platform! Assumptions have been made for Windows which cannot be made for other platforms.
#endif
    std::wstring ret = convert.from_bytes(str.AsCharPtr());
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void*
LoadAssemblyAndGetExport(Util::String const& configPath)
{
    // Load .NET Core
    void* load_assembly_and_get_function_pointer = nullptr;
    hostfxr_handle cxt = nullptr;

    int rc = api.InitConfig(ToWideString(configPath).c_str(), nullptr, &cxt);
    if (((rc != Success && rc != Success_HostAlreadyInitialized && rc != Success_DifferentRuntimeProperties) && cxt == nullptr))
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
    return (load_assembly_and_get_function_pointer_fn)load_assembly_and_get_function_pointer;
}

//------------------------------------------------------------------------------
/**
*/
void*
LoadAssemblyAndGetExport(Util::CommandLineArgs const& args)
{
    // Load .NET Core
    void* load_assembly_and_get_function_pointer = nullptr;
    hostfxr_handle cxt = nullptr;

    // TEMP
    const char_t* argv[] = {
        L"C:/Users/fredrik/git/nebula_workspace/fips-deploy/nebula/vulkan-win64-vstudio-debug/NSharpTests.dll"};

    int rc = api.InitCommandLine(1, argv, nullptr, &cxt);
    if ((rc != 0 || cxt == nullptr))
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
    return (load_assembly_and_get_function_pointer_fn)load_assembly_and_get_function_pointer;
}

//------------------------------------------------------------------------------
/**
*/
bool
NSharpServer::Open()
{
    n_assert(!this->IsOpen());
    // Host custom .net runtime
    bool res = this->LoadHostFxr();
    n_assert(res);

    this->isOpen = true;
    return true;
}

//------------------------------------------------------------------------------
/**
*/
void
NSharpServer::Close()
{
    n_assert(this->IsOpen());

    this->CloseHostFxr();

    this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void
NSharpServer::SetDebuggingEnabled(bool enabled)
{
    this->debuggerEnabled = enabled;
}

//------------------------------------------------------------------------------
/**
*/
void
NSharpServer::WaitForDebuggerToConnect(bool enabled)
{
    this->waitForDebugger = enabled;
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

    assembly->dllPath = ToWideString(uri.GetHostAndLocalPath());
    assembly->name = uri.AsString().ExtractFileName();
    assembly->name.StripFileExtension();

    AssemblyId assemblyId = this->assemblies.Alloc();
    this->assemblies.Get<0>(assemblyId.id) = assembly;
    this->assemblyTable.Add(assembly->name, assemblyId.id);
    return assemblyId;
}

//------------------------------------------------------------------------------
/**
	Function should be formatted as: "Namespace.Namespace.Class+NestedClass::Function()"
*/
int
NSharpServer::ExecUnmanagedCall(AssemblyId assemblyId, Util::String const& function)
{
    if (assemblyId > this->assemblies.Size() || assemblyId == InvalidIndex)
    {
        n_warning("Invalid assembly id!");
        return 1;
    }

    Assembly* assembly = this->assemblies.Get<0>(assemblyId.id);

    // Separate class and namespace from string.
    auto methodSeparatorIndex = function.FindCharIndex(':');
    Util::String ns = function.ExtractRange(0, methodSeparatorIndex);
    ns += ", ";
    ns += assembly->name;
    Util::String methodName = function.ExtractRange(methodSeparatorIndex + 2, function.Length() - methodSeparatorIndex - 4);

    std::wstring dotnetType = ToWideString(ns);
    std::wstring dotnetTypeMethod = ToWideString(methodName);

    typedef void(CORECLR_DELEGATE_CALLTYPE * custom_entry_point_fn)();
    custom_entry_point_fn func = nullptr;

    int rc = assembly->GetExport(
        assembly->dllPath.c_str(),
        dotnetType.c_str(),
        dotnetTypeMethod.c_str(),
        UNMANAGEDCALLERSONLY_METHOD,
        nullptr,
        (void**)&func
    );

    if (rc != 0 || func == nullptr)
    {
        n_warning("Could not find function '%s' in assembly (%s)!", function.AsCharPtr(), assembly->name.AsCharPtr());
        n_printf("\tPossible solutions:\n"
                 "\t\tFunction name is correctly formatted (ex. Namespace.Namespace.Class+NestedClass::Function())?\n"
                 "\t\tFunction is declared with the [UnmanagedCallersOnly] attribute in C#?");
        return 2;
    }

    func();

    return 0;
}

//------------------------------------------------------------------------------
/**
*/
bool const
NSharpServer::IsOpen()
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
AssemblyId
NSharpServer::GetCoreAssembly() const
{
    return this->nebulaEngineAssemblyId;
}

} // namespace Scripting
