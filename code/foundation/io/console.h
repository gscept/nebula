#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::Console
    
    Nebula's console, this is the central place for command-
    line-style communication with the user. By default, all output will
    just disappear unless console handlers are added. Console handlers
    are user-derivable objects which do something with the output and
    may provide text input.

    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"
#include "util/array.h"
#include "core/refcounted.h"
#include "core/singleton.h"
#include "core/ptr.h"
#include "io/consolehandler.h"
#include "threading/threadid.h"
#include "threading/criticalsection.h"

//------------------------------------------------------------------------------
namespace IO
{
class Console : public Core::RefCounted
{
    __DeclareClass(Console);
    __DeclareInterfaceSingleton(Console);
public:
    /// constructor
    Console();
    /// destructor
    virtual ~Console();
    /// open the console
    void Open();
    /// close the console
    void Close();
    /// return true if currently open
    bool IsOpen() const;
    /// called per-frame
    void Update();
    /// attach a console handler to the console
    void AttachHandler(const Ptr<ConsoleHandler>& handler);
    /// remove a console handler from the console
    void RemoveHandler(const Ptr<ConsoleHandler>& handler);
    /// get array of currently installed handlers
    Util::Array<Ptr<ConsoleHandler> > GetHandlers() const;
    /// return true if user input is available
    bool HasInput() const;
    /// get user input
    Util::String GetInput() const;
    /// print a formatted line (printf style)
    void __cdecl Print(const char* fmt, ...);
    /// print a formatted line (printf style)
    void __cdecl Print(const char* fmt, va_list argList);
    /// print a string object
    void Print(const Util::String& s);
    /// put an error message and cancel execution
    void __cdecl Error(const char* fmt, ...);
    /// put an error message and cancel execution
    void __cdecl Error(const char* fmt, va_list argList);
    /// put a warning message
    void __cdecl Warning(const char* fmt, ...);
    /// put a warning message
    void __cdecl Warning(const char* fmt, va_list argList);
    /// display a confirmation message box
    void __cdecl Confirm(const char* fmt, ...);
    /// display a confirmation message box
    void __cdecl Confirm(const char* fmt, va_list argList);
    /// print a debug-only message
    void __cdecl DebugOut(const char* fmt, ...);
    /// print a debug-only message
    void __cdecl DebugOut(const char* fmt, va_list argList);

protected:
    Threading::ThreadId creatorThreadId;
    Threading::CriticalSection critSect;
    Util::Array<Ptr<ConsoleHandler> > consoleHandlers;
    bool isOpen;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
Console::IsOpen() const
{
    return this->isOpen;
}

//------------------------------------------------------------------------------
/**
*/
inline Util::Array<Ptr<ConsoleHandler> >
Console::GetHandlers() const
{
    this->critSect.Enter();
    Util::Array<Ptr<ConsoleHandler> > handlers = this->consoleHandlers;
    this->critSect.Leave();
    return handlers;
}

} // namespace IO
//------------------------------------------------------------------------------
