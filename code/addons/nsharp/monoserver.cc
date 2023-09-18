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

using namespace IO;

namespace Scripting
{
__ImplementClass(Scripting::MonoServer, 'MONO', Core::RefCounted);
__ImplementSingleton(Scripting::MonoServer);

using namespace Util;
using namespace IO;

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

	if (debuggerEnabled && waitForDebugger)
	{
		static const char* options[] = {
			"--debugger-agent=address=0.0.0.0:55555,transport=dt_socket,server=y",
			"--soft-breakpoints",
			// "--trace"
		};
		mono_jit_parse_options(sizeof(options) / sizeof(char*), (char**)options);
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

	// Intialize JIT runtime
	this->domain = mono_jit_init("Nebula Mono Subsystem");
	if (!domain)
	{
		n_error("Failed to initialize Mono JIT runtime!");
		return false;
	}

	if (debuggerEnabled)
	{
		mono_debug_init(MONO_DEBUG_FORMAT_MONO);
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
