#pragma once
#ifndef POSIX_POSIXCONSOLEHANDLER_H
#define POSIX_POSIXCONSOLEHANDLER_H
//------------------------------------------------------------------------------
/**
    @class Posix::PosixConsoleHandler
    
    The default console handler for Posix, puts normal messages to
    the debug output channel, and error messages into a message box.
    Does not provide any input.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file    
*/
#include "io/consolehandler.h"

//------------------------------------------------------------------------------
namespace Posix
{
class PosixConsoleHandler : public IO::ConsoleHandler
{
    __DeclareClass(PosixConsoleHandler);
public:
    /// constructor
    PosixConsoleHandler();
    
    /// called by console to output data
    virtual void Print(const Util::String& s);
    /// called by console with serious error
    virtual void Error(const Util::String& s);
    /// called by console to output warning
    virtual void Warning(const Util::String& s);
    /// called by console to output debug string
    virtual void DebugOut(const Util::String& s);
    /// return true if input is available
    virtual bool HasInput();
    /// read available input
    virtual Util::String GetInput();

private:
    FILE * stdoutHandle;
    FILE * stdinHandle;
    FILE * stderrHandle;
};

}; // namespace Posix
//------------------------------------------------------------------------------
#endif
