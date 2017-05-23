#pragma once
//------------------------------------------------------------------------------
/**
    @class OSX::OSXConsoleHandler
 
    The default console handler for OSX, puts messages to stdout and stderr,
    reads from stdin.
 
    (C) 2010 Radon Labs GmbH
    (C) 2013 Individual contributors, see AUTHORS file    
 */
#include "io/consolehandler.h"

//------------------------------------------------------------------------------
namespace OSX
{
class OSXConsoleHandler : public IO::ConsoleHandler
{
    __DeclareClass(OSXConsoleHandler);
public:
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
};
    
} // namespace OSX
