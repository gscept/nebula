//------------------------------------------------------------------------------
//  sysfunc.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "core/win32/win32sysfunc.h"
#include "core/refcounted.h"
#include "debug/minidump.h"
#include "util/blob.h"
#include "net/socket/socket.h"
#include "debug/minidump.h"
#include "threading/thread.h"
#include "util/globalstringatomtable.h"
#include "util/localstringatomtable.h"  
#include "system/systeminfo.h"
#include "debug/win32/win32stacktrace.h"
#include <io.h>

namespace Win32
{
using namespace Util;

bool volatile SysFunc::SetupCalled = false;
const Core::ExitHandler* SysFunc::ExitHandlers = 0;
System::SystemInfo SysFunc::systemInfo;

Util::GlobalStringAtomTable* globalStringAtomTable = 0;
#if NEBULA_ENABLE_THREADLOCAL_STRINGATOM_TABLES
    Util::LocalStringAtomTable* localStringAtomTable = 0;
#endif

//------------------------------------------------------------------------------
/**
    This method must be called at application start before any threads
    are spawned. It is used to setup static objects beforehand (i.e.
    private heaps of various utility classes). Doing this eliminates
    the need for fine-grained locking in the affected classes.
*/
void
SysFunc::Setup()
{
    if (!SetupCalled)
    {
        SetupCalled = true;
        #if !__MAYA__
        Threading::Thread::SetMyThreadName("MainThread");
        #endif
        Memory::SetupHeaps();
        Memory::Heap::Setup();
        Blob::Setup();
        #if !__MAYA__
        Net::Socket::InitNetwork();
        Debug::MiniDump::Setup();
        #endif   

        globalStringAtomTable = new Util::GlobalStringAtomTable;
        #if NEBULA_ENABLE_THREADLOCAL_STRINGATOM_TABLES
            localStringAtomTable = new Util::LocalStringAtomTable;
        #endif    

        // query simd support
        int CPUInfo[4] = { -1 };
        __cpuid(CPUInfo, 0x00000001);
#if N_USE_AVX
        if ((CPUInfo[2] & (1 << 28)) != (1 << 28))
            n_error("Error: No AVX support");
#endif
        if ((CPUInfo[2] & (1 << 20)) != (1 << 20))
            n_error("Error: No SSE4 support");
        if ((CPUInfo[3] & (1 << 26)) != (1 << 26))
            n_error("Error: No SSE2 support... wtf?");

#if N_USE_FMA
        if ((CPUInfo[2] & (1 << 12)) != (1 << 12))
            n_error("No FMA support");
#endif

    }
}

//------------------------------------------------------------------------------
/**
    This method is called by Application::Exit(), or otherwise must be
    called right before the end of the programs main() function. The method
    will properly shutdown the Nebula runtime environment, and report 
    refcounting and memory leaks (debug builds only). This method will not
    return.
*/
void
SysFunc::Exit(int exitCode)
{
    // delete string atom tables
    delete globalStringAtomTable;
    #if NEBULA_ENABLE_THREADLOCAL_STRINGATOM_TABLES
        delete localStringAtomTable;
    #endif

    // first produce a RefCount leak report
    #if NEBULA_DEBUG
    Core::RefCounted::DumpRefCountingLeaks();
    #endif

    // call exit handlers
    const Core::ExitHandler* exitHandler = SysFunc::ExitHandlers;
    while (0 != exitHandler)
    {
        exitHandler->OnExit();
        exitHandler = exitHandler->Next();
    }

    // shutdown the C runtime, this cleans up static objects but doesn't shut 
    // down the process
    _cexit();

    // call static shutdown methods
    Blob::Shutdown();

    // shutdown global factory object
    Core::Factory::Destroy();

    // report mem leaks
    #if NEBULA_MEMORY_ADVANCED_DEBUGGING
    Memory::DumpMemoryLeaks();
    #endif   

    // finally terminate the process
    ExitProcess(exitCode);
}

//------------------------------------------------------------------------------
/**
    This displays a Win32 error message box and quits the program
*/
void
SysFunc::Error(const char* error)
{
    #ifdef _DEBUG
    OutputDebugString(error);   
    #endif
    /*
    HWND hwnd = FindWindow(NEBULA_WINDOW_CLASS, NULL);
    if (hwnd)
    {
        ShowWindow(hwnd, SW_MINIMIZE);
    }
    */
    Util::Array<Util::String> stacktrace = Win32StackTrace::GenerateStackTrace();
    Util::String format;
    // remove the first 7 entries as they are only the assert/error functions and the last 6 as they are windows startup 
    for (int i = 6; i < Math::min(17,stacktrace.Size() - 6); i++)
    {
        format.Append(stacktrace[i]);
        //format.Append("\n");
    }
    format.Format("%s\n\
----- *** CALL STACK *** -----\n\
%s\
----- *** CALL STACK *** -----\n\n", error, format.AsCharPtr());

    if(_isatty(_fileno(stdout)))
    {
#ifdef _DEBUG
        if (IsDebuggerPresent())
        {
            OutputDebugString(format.AsCharPtr());
            fprintf(stderr, "%s", format.AsCharPtr());
            n_break();
            exit(1);
        }
        else
#endif
        {
            ::MessageBox(NULL, format.AsCharPtr(), "NEBULA SYSTEM ERROR", MB_OK | MB_APPLMODAL | MB_SETFOREGROUND | MB_TOPMOST | MB_ICONERROR);
#if !__MAYA__
            Debug::MiniDump::WriteMiniDump();
#endif
            abort();
        }
    }
    else
    {
#ifdef _DEBUG
        OutputDebugString(format.AsCharPtr());
        fprintf(stderr,"%s\n",format.AsCharPtr());        
        if (IsDebuggerPresent())
        { 
            n_break();
        }        
#else
        exit(1);
#endif
    }
    
}

//------------------------------------------------------------------------------
/**
    This displays a Win32 message box
*/
void
SysFunc::MessageBox(const char* msg)
{
    #ifdef _DEBUG
    OutputDebugString(msg);
    #endif
    HWND hwnd = FindWindow(NEBULA_WINDOW_CLASS, NULL);
    if (hwnd)
    {
        ShowWindow(hwnd, SW_MINIMIZE);
    }
    ::MessageBox(NULL, msg, "NEBULA MESSAGE", MB_OK|MB_APPLMODAL|MB_SETFOREGROUND|MB_TOPMOST|MB_ICONINFORMATION);
}

//------------------------------------------------------------------------------
/**
    Sleep for a specified amount of seconds, give up time slice.
*/
void
SysFunc::Sleep(double sec)
{
    int milliSecs = int(sec * 1000.0);
    ::Sleep(milliSecs);
}

//------------------------------------------------------------------------------
/**
    Put a message on the debug console.
*/
void
SysFunc::DebugOut(const char* msg)
{
    OutputDebugString(msg);
}

//------------------------------------------------------------------------------
/**
    Register a new exit handler. This method is called at startup time
    from the constructor of static exit handler objects. This is the only
    supported way to register exit handlers. The method will return
    a pointer to the next exit handler in the forward linked list 
    (or 0 if this is the first exit handler).
*/
const Core::ExitHandler*
SysFunc::RegisterExitHandler(const Core::ExitHandler* exitHandler)
{
    const Core::ExitHandler* firstHandler = ExitHandlers;
    ExitHandlers = exitHandler;
    return firstHandler;
}

//------------------------------------------------------------------------------
/**
*/
const System::SystemInfo*
SysFunc::GetSystemInfo()
{
    return &SysFunc::systemInfo;
}


} // namespace Win32
