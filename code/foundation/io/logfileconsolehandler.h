#pragma once
//------------------------------------------------------------------------------
/**
    @class IO::LogFileConsoleHandler
    
    A console handler which writes all console output to a log file. The
    log file will be called appname_calendartime.txt and will be written
    into a bin:logfiles directory.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "io/consolehandler.h"
#include "io/stream.h"
#include "io/textwriter.h"

//------------------------------------------------------------------------------
namespace IO
{
class LogFileConsoleHandler : public ConsoleHandler
{
    __DeclareClass(LogFileConsoleHandler);
public:
    /// constructor
    LogFileConsoleHandler();
    /// destructor
    virtual ~LogFileConsoleHandler();
    
    /// called by console when attached
    virtual void Open();
    /// called by console when removed
    virtual void Close();

    /// called by console to output data
    virtual void Print(const Util::String& s);
    /// called by console with serious error
    virtual void Error(const Util::String& s);
    /// called by console to output warning
    virtual void Warning(const Util::String& s);
    /// called by console to output debug string
    virtual void DebugOut(const Util::String& s);

private:
    Ptr<Stream> stream;
    Ptr<TextWriter> textWriter;
};

} // namespace IO
//------------------------------------------------------------------------------
