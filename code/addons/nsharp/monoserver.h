#pragma once
//------------------------------------------------------------------------------
/**
    @class Scripting::MonoServer
  
    Mono backend for the Nebula scripting subsystem.
  
    Main goals for the nSharp library:
    Fast compilation times.
    Hot reloading.

    (C) 2019 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "core/singleton.h"
#include "util/string.h"
#include "ids/id.h"
#include "io/uri.h"
#include "util/arrayallocator.h"
#include "util/dictionary.h"
//#include "monobindings.h"
#include "system/library.h"
#include "monoconfig.h"

//------------------------------------------------------------------------------
namespace Scripting
{

ID_16_TYPE(NSharpAssemblyId);

class MonoServer : public Core::RefCounted
{
    __DeclareClass(MonoServer);
    __DeclareSingleton(MonoServer);
public:
    /// constructor
	MonoServer();
    /// destructor
    virtual ~MonoServer();
    /// open the script server
    bool Open();
    /// close the script server
    void Close();
    /// enable debugging. this needs to be called before Open()
	void SetDebuggingEnabled(bool enabled);
	void WaitForDebuggerToConnect(bool enabled);
	/// Load mono exe or DLL at path
	NSharpAssemblyId LoadAssembly(IO::URI const& uri);
	/// Execute function in an assembly. This is not very efficient and should not be used in place of delegates
	int ExecUnmanagedCall(NSharpAssemblyId assembly, Util::String const& function);
	/// Check if mono server is open
	bool const IsOpen();
private:
    struct Assembly;

    /// load the host fxr library and get exported function addresses
    bool LoadHostFxr();
    void CloseHostFxr();

    //Mono::MonoBindings bindings;

    Util::ArrayAllocator<Assembly*> assemblies;
	Util::Dictionary<Util::String, uint32_t> assemblyTable;

    bool debuggerEnabled;
	bool waitForDebugger;

	bool isOpen;
    System::Library hostfxr;
};

} // namespace Scripting
//------------------------------------------------------------------------------
