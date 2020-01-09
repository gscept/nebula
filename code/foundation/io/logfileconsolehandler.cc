//------------------------------------------------------------------------------
//  logfileconsolehandler.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/logfileconsolehandler.h"
#include "io/ioserver.h"
#include "timing/calendartime.h"
#include "core/coreserver.h"

namespace IO
{
__ImplementClass(IO::LogFileConsoleHandler, 'LFCH', IO::ConsoleHandler);

using namespace Util;
using namespace Timing;

//------------------------------------------------------------------------------
/**
*/
LogFileConsoleHandler::LogFileConsoleHandler()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
LogFileConsoleHandler::~LogFileConsoleHandler()
{
    if (this->IsOpen())
    {
        this->Close();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
LogFileConsoleHandler::Open()
{
    ConsoleHandler::Open();
    IoServer* ioServer = IoServer::Instance();

    // make sure log directory exists
    ioServer->CreateDirectory("bin:logfiles");

    // build a file name for the log file
    String calString = CalendarTime::Format("{YEAR}_{MONTH}_{DAY}_{HOUR}_{MINUTE}_{SECOND}", CalendarTime::GetLocalTime());
    String fileName;
    fileName.Format("bin:logfiles/%s_%s.txt", Core::CoreServer::Instance()->GetAppName().Value(), calString.AsCharPtr());

    // open file stream
    this->stream = ioServer->CreateStream(URI(fileName));
    this->textWriter = TextWriter::Create();
    this->textWriter->SetStream(this->stream);
    this->textWriter->Open();
}

//------------------------------------------------------------------------------
/**
*/
void
LogFileConsoleHandler::Close()
{
    n_assert(this->IsOpen());

    if (this->textWriter->IsOpen())
    {
        this->textWriter->Close();
    }
    this->textWriter = nullptr;
    this->stream = nullptr;
    
    ConsoleHandler::Close();
}

//------------------------------------------------------------------------------
/**
*/
void
LogFileConsoleHandler::Print(const String& s)
{
    if (this->textWriter->IsOpen())
    {
        this->textWriter->WriteString(s);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
LogFileConsoleHandler::Error(const String& s)
{
    if (this->textWriter->IsOpen())
    {
        String str("[ERROR] ");
        str.Append(s);
        this->textWriter->WriteString(str);
        this->stream->Flush();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
LogFileConsoleHandler::Warning(const String& s)
{
    if (this->textWriter->IsOpen())
    {
        String str("[WARNING] ");
        str.Append(s);
        this->textWriter->WriteString(str);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
LogFileConsoleHandler::DebugOut(const String& s)
{
    if (this->textWriter->IsOpen())
    {
        String str("[DEBUGOUT] ");
        str.Append(s);
        this->textWriter->WriteString(str);
    }
}

} // namespace IO
