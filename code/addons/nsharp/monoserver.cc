//------------------------------------------------------------------------------
//  monoserver.cc
//  (C) 2019 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "monoserver.h"
#include "io/ioserver.h"
#include "io/textreader.h"
#include <mono/jit/jit.h>
#include <mono/metadata/assembly.h>
#include "mono/metadata/mono-config.h"
#include "mono/utils/mono-error.h"
#include "mono/metadata/debug-helpers.h"
#include "mono/metadata/mono-debug.h"
#include "mono/utils/mono-logger.h"
#include "debug/debugserver.h"
#include "monobindings.h"
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
__ImplementClass(Scripting::MonoServer, 'MONO', Core::RefCounted);
__ImplementSingleton(Scripting::MonoServer);

using namespace Util;
using namespace IO;

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
MonoServer::MonoServer() :
    isOpen(false),
    waitForDebugger(false),
    debuggerEnabled(false)
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
MonoServer::~MonoServer()
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
MonoServer::LoadHostFxr()
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

    api = {
        .InitConfig=(hostfxr_initialize_for_runtime_config_fn)this->hostfxr.GetExport("hostfxr_initialize_for_runtime_config"),
        .InitCommandLine=(hostfxr_initialize_for_dotnet_command_line_fn)this->hostfxr.GetExport("hostfxr_initialize_for_dotnet_command_line"),
        .GetDelegate=(hostfxr_get_runtime_delegate_fn)this->hostfxr.GetExport("hostfxr_get_runtime_delegate"),
        .Close=(hostfxr_close_fn)this->hostfxr.GetExport("hostfxr_close")
    };

    return api.Validate();
}

//------------------------------------------------------------------------------
/**
*/
void
MonoServer::CloseHostFxr()
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
LoadAssemblyAndGetExport(IO::URI const& configPath)
{
    // Load .NET Core
    void* load_assembly_and_get_function_pointer = nullptr;
    hostfxr_handle cxt = nullptr;
    Util::String const path = configPath.GetHostAndLocalPath();

    int rc = api.InitConfig(ToWideString(path).c_str(), nullptr, &cxt);
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
void*
LoadAssemblyAndGetExport(Util::CommandLineArgs const& args)
{
    // Load .NET Core
    void* load_assembly_and_get_function_pointer = nullptr;
    hostfxr_handle cxt = nullptr;
    
    // TEMP
    const char_t* argv[] = {L"C:/Users/fredrik/git/nebula_workspace/fips-deploy/nebula/vulkan-win64-vstudio-debug/NebulaEngine.dll"};

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
MonoServer::Open()
{
    n_assert(!this->IsOpen());    
    
	/// Setup a custom allocator for unmanaged memory allocated in CLR
	MonoAllocatorVTable mem_vtable = { 
		.version=1,
		.malloc=ScriptingAlloc,
		.realloc=ScriptingRealloc,
		.free=ScriptingDealloc,
		.calloc=ScriptingCalloc
	};
		
	if (!mono_set_allocator_vtable(&mem_vtable))
		n_warning("Mono allocator not set!");

	mono_config_parse(NULL);

    if (debuggerEnabled)
    {
        if (waitForDebugger)
        {
            static const char* options[] = {
                "--debugger-agent=address=0.0.0.0:55555,transport=dt_socket,server=y",
                "--soft-breakpoints",
                // "--trace"
            };
            mono_jit_parse_options(sizeof(options) / sizeof(char*), (char**)options);
        }
    }

    // TODO: We should bundle release with the needed libraries and configs for mono
#if (__WIN32__)
    mono_set_dirs("C:\\Program Files\\Mono\\lib", "C:\\Program Files\\Mono\\etc");
#elif ( __OSX__ || __APPLE__ || __linux__ )
    // TODO: untested, temporary paths
    mono_set_dirs("/bin/mono/lib", "/etc/mono/");
#else
#error "MONO NOT YET SUPPORTED ON PROVIDED PLATFORM"
#endif

    if (debuggerEnabled)
    {
        mono_debug_init(MONO_DEBUG_FORMAT_MONO);
    }

	// Intialize JIT runtime
	this->domain = mono_jit_init("Nebula Mono Subsystem");
	if (!domain)
	{
		n_error("Failed to initialize Mono JIT runtime!");
		return false;
	}

    if (debuggerEnabled)
    {
        mono_debug_domain_create(this->domain);
    }

	mono_trace_set_log_handler(Mono::N_Log, nullptr);
	mono_trace_set_print_handler(Mono::N_Print);
	mono_trace_set_printerr_handler(Mono::N_Error);

	IO::URI uri = IO::URI("bin:NebulaEngine.dll");
	Util::String path = uri.AsString();

	// setup default executable
	MonoAssembly* assembly;
	assembly = mono_domain_assembly_open(domain, path.AsCharPtr());
	if (!assembly)
	{
		n_error("Mono initialization: Could not load Mono assembly!");
		return false;
	}

	mono_assembly_set_main(assembly);

	MonoImage* image = mono_assembly_get_image(assembly);

	MonoClass* cls = mono_class_from_name(image, "Nebula", "AppEntry");

	MonoMethodDesc* desc = mono_method_desc_new(":Main()", false);
	MonoMethod* entryPoint = mono_method_desc_search_in_class(desc, cls);

	if (!entryPoint)
	{
		n_error("Could not find entry point for Mono scripts!");
		return false;
	}

	bindings = Mono::MonoBindings(image);
	bindings.Initialize();

	mono_runtime_invoke(entryPoint, NULL, NULL, NULL);

    // Host custom .net runtime
    bool res = this->LoadHostFxr();
    n_assert(res);

    const IO::URI configPath = ("bin:NebulaEngine.runtimeconfig.json");
    load_assembly_and_get_function_pointer_fn load_assembly_and_get_function_pointer = nullptr;

    //Util::CommandLineArgs clArgs;
    //load_assembly_and_get_function_pointer = (load_assembly_and_get_function_pointer_fn)LoadAssemblyAndGetExport(clArgs);

    load_assembly_and_get_function_pointer = (load_assembly_and_get_function_pointer_fn)LoadAssemblyAndGetExport(configPath);

    n_assert2(load_assembly_and_get_function_pointer != nullptr, "Failure: LoadAssemblyAndGetExport()");

	this->isOpen = true;
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
MonoServer::Close()
{
    n_assert(this->IsOpen());    
    
	// shutdown mono runtime
	mono_jit_cleanup(this->domain);

    this->CloseHostFxr();

	this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
*/
void
MonoServer::SetDebuggingEnabled(bool enabled)
{
	this->debuggerEnabled = enabled;
}

//------------------------------------------------------------------------------
/**
*/
void
MonoServer::WaitForDebuggerToConnect(bool enabled)
{
    this->waitForDebugger = enabled;
}

//------------------------------------------------------------------------------
/**
*/
MonoAssemblyId
MonoServer::Load(IO::URI const& uri)
{
	Util::String path = uri.AsString();
	MonoAssembly* assembly = mono_domain_assembly_open(domain, path.AsCharPtr());
	
	if (!assembly)
	{
		n_warning("Could not load Mono assembly!");
		return InvalidIndex;
	}

	MonoAssemblyId assemblyId = this->assemblies.Alloc();
	this->assemblies.Get<0>(assemblyId.id) = assembly;
	this->assemblyTable.Add(path, assemblyId.id);
	return assemblyId;
}

//------------------------------------------------------------------------------
/**
	Function should be formatted as: "Namespace.Namespace.Class/NestedClass::Function()"
*/
int
MonoServer::Exec(MonoAssemblyId assemblyId, Util::String const& function)
{
	if (assemblyId > this->assemblies.Size() || assemblyId == InvalidIndex)
	{
		n_warning("Invalid assembly id!");
		return 1;
	}

	MonoAssembly* assembly = this->assemblies.Get<0>(assemblyId.id);
	MonoImage* image = mono_assembly_get_image(assembly);

	// Separate class and namespace from string.
	auto methodSeparatorIndex = function.FindCharIndex(':');
	Util::String ns = function.ExtractRange(0, methodSeparatorIndex);
	Util::String clsName = ns.GetFileExtension();
	ns = ns.ExtractRange(0, ns.Length() - clsName.Length() - 1);

	MonoClass* cls = mono_class_from_name(image, ns.AsCharPtr(), clsName.AsCharPtr());

	MonoMethodDesc* desc = mono_method_desc_new(function.AsCharPtr(), true);
	MonoMethod* entryPoint = mono_method_desc_search_in_class(desc, cls);

	if (!entryPoint)
	{
		n_warning("Could not find entry point for Mono scripts!");
		return 2;
	}

	mono_runtime_invoke(entryPoint, NULL, NULL, NULL);

    return 0;
}

//------------------------------------------------------------------------------
/**
*/
bool const
MonoServer::IsOpen()
{
	return this->isOpen;
}

} // namespace Scripting
