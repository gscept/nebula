#pragma once
//------------------------------------------------------------------------------
/**
    @class OSX::SysFunc

    Lowest-level functions for OSX platform.

    (C) 2010 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file    
*/
#include "core/types.h"
#include "core/exithandler.h"

namespace System
{
class SystemInfo;
}

//------------------------------------------------------------------------------
namespace OSX
{
class SysFunc
{
public:
    /// setup lowlevel Nebula3 runtime (called before anything else)
    static void Setup();
    /// cleanly exit the process
    static void Exit(int exitCode);
    /// display an error message
    static void Error(const char* error);
    /// display a message which must be confirmed by the user
    static void MessageBox(const char* msg);
    /// print a message on the debug concole
    static void DebugOut(const char* msg);
    /// sleep for a specified amount of seconds
    static void Sleep(double sec);    
    /// get system information
    static const System::SystemInfo* GetSystemInfo();
    
private:
    friend class Core::ExitHandler;
    /// register an exit handler which will be called from within Exit()
    static const Core::ExitHandler* RegisterExitHandler(const Core::ExitHandler* exitHandler);
    
    static bool volatile SetupCalled;
    static const Core::ExitHandler* ExitHandlers;     // forward linked list of exit handlers
    static System::SystemInfo systemInfo;
};
    
} // namespace OSX
//------------------------------------------------------------------------------

