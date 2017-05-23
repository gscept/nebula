#pragma once
#ifndef WIN32_WIN32CONSOLEHANDLER_H
#define WIN32_WIN32CONSOLEHANDLER_H
//------------------------------------------------------------------------------
/**
    @class Win32::Win32ConsoleHandler
    
    The default console handler for Win32, puts normal messages to
    the debug output channel, and error messages into a message box.
    Does not provide any input.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "io/consolehandler.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32ConsoleHandler : public IO::ConsoleHandler
{
    __DeclareClass(Win32ConsoleHandler);
public:
    /// constructor
    Win32ConsoleHandler();
    
    /// called by console to output data
    virtual void Print(const Util::String& s);
    /// called by console with serious error
    virtual void Error(const Util::String& s);
    /// called by console to output warning
    virtual void Warning(const Util::String& s);
    /// called by console to display confirmation message box
    virtual void Confirm(const Util::String& s);
    /// called by console to output debug string
    virtual void DebugOut(const Util::String& s);
    /// return true if input is available
    virtual bool HasInput();
    /// read available input
    virtual Util::String GetInput();

private:
    HANDLE stdoutHandle;
    HANDLE stdinHandle;
    HANDLE stderrHandle;
};

}; // namespace Win32
//------------------------------------------------------------------------------
#endif