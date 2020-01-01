//------------------------------------------------------------------------------
//  historyconsolehandler.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "io/historyconsolehandler.h"

namespace IO
{
__ImplementClass(IO::HistoryConsoleHandler, 'HICH', IO::ConsoleHandler);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
HistoryConsoleHandler::HistoryConsoleHandler() :
    history(NEBULA_CONSOLE_HISTORY_SIZE)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
HistoryConsoleHandler::Print(const String& s)
{
    this->history.Add(s);
}

//------------------------------------------------------------------------------
/**
*/
void
HistoryConsoleHandler::Error(const String& s)
{
    String str("[ERROR] ");
    str.Append(s);
    this->history.Add(str);
}

//------------------------------------------------------------------------------
/**
*/
void
HistoryConsoleHandler::Warning(const String& s)
{
    String str("[WARNING] ");
    str.Append(s);
    this->history.Add(str);
}

//------------------------------------------------------------------------------
/**
*/
void
HistoryConsoleHandler::DebugOut(const String& s)
{
    String str("[DEBUG] ");
    str.Append(s);
    this->history.Add(str);
}
        
} // namespace IO
