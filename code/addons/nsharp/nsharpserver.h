#pragma once
//------------------------------------------------------------------------------
/**
    @class Scripting::NSharpServer
  
    C# backend for the Nebula scripting subsystem.
  
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
#include "system/library.h"
#include "nsconfig.h"
#include "assemblyid.h"

//------------------------------------------------------------------------------
namespace Scripting
{

class NSharpServer : public Core::RefCounted
{
    __DeclareClass(NSharpServer);
    __DeclareSingleton(NSharpServer);
public:
    /// constructor
	NSharpServer();
    /// destructor
    virtual ~NSharpServer();
    /// open the script server
    bool Open();
    /// close the script server
    void Close();
    /// enable debugging. this needs to be called before Open()
	void SetDebuggingEnabled(bool enabled);
	void WaitForDebuggerToConnect(bool enabled);
	/// Load dotnet exe or DLL at path
	AssemblyId LoadAssembly(IO::URI const& uri);
	/// Execute function in an assembly. This is not very efficient and should not be used in place of delegates
	int ExecUnmanagedCall(AssemblyId assembly, Util::String const& function);
	/// Check if server is open
	bool const IsOpen();
    /// Get the nebula engine api core assembly that contains the nebula C# runtime
    AssemblyId GetCoreAssembly() const;

private:
    struct Assembly;

    /// load the host fxr library and get exported function addresses
    bool LoadHostFxr();
    void CloseHostFxr();

    AssemblyId nebulaEngineAssemblyId;

    Util::ArrayAllocator<Assembly*> assemblies;
	Util::Dictionary<Util::String, uint32_t> assemblyTable;

    bool debuggerEnabled;
	bool waitForDebugger;

	bool isOpen;
    System::Library hostfxr;
};

} // namespace Scripting
//------------------------------------------------------------------------------
