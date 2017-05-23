//------------------------------------------------------------------------------
//  win32consolehandler.cc
//  (C) 2010 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "win32consolehandler.h"
#include "core/coreserver.h"
#include "core/sysfunc.h"
#include "threading/thread.h"

namespace Win32
{
__ImplementClass(Win32::Win32ConsoleHandler, 'W32C', IO::ConsoleHandler);

using namespace Util;

//------------------------------------------------------------------------------
/**
*/
Win32ConsoleHandler::Win32ConsoleHandler()
{
    // obtain handle to stdout
    this->stdoutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
    this->stdinHandle = GetStdHandle(STD_INPUT_HANDLE);
    this->stderrHandle = GetStdHandle(STD_ERROR_HANDLE);
    n_assert(INVALID_HANDLE_VALUE != this->stdoutHandle);
    n_assert(INVALID_HANDLE_VALUE != this->stdinHandle);
    n_assert(INVALID_HANDLE_VALUE != this->stderrHandle);
}

//------------------------------------------------------------------------------
/**
*/
void
Win32ConsoleHandler::Print(const String& s)
{
    DWORD charsWritten;
    DWORD out;
    if (GetConsoleMode(this->stdoutHandle, &out))
    {
        WriteConsole(this->stdoutHandle, s.AsCharPtr(), s.Length(), &charsWritten, NULL);
    }
    else
    {
        WriteFile(this->stdoutHandle, s.AsCharPtr(), s.Length(), &charsWritten, NULL);
    }
    #ifndef PUBLIC_BUILD
    OutputDebugString(s.AsCharPtr());
    #endif
}

//------------------------------------------------------------------------------
/**
*/
void
Win32ConsoleHandler::DebugOut(const String& s)
{
    OutputDebugString(s.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
void
Win32ConsoleHandler::Error(const String& msg)
{
    const char* threadName = Threading::Thread::GetMyThreadName();
    const char* appName = "???";
    if (0 == threadName)
    {
        threadName = "Main";
    }
    if (Core::CoreServer::HasInstance())
    {
        appName = Core::CoreServer::Instance()->GetAppName().Value();
    }
    String str;
    str.Format("*** ERROR ***\nApplication: %s\nThread: %s\nError: %s", appName, threadName, msg.AsCharPtr());
    DWORD charsWritten;
    WriteConsole(this->stderrHandle, str.AsCharPtr(), str.Length(), &charsWritten, NULL);
    Core::SysFunc::Error(str.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
*/
void
Win32ConsoleHandler::Warning(const String& s)
{
    DWORD charsWritten;
    WriteConsole(this->stderrHandle, s.AsCharPtr(), s.Length(), &charsWritten, NULL);
    #ifdef _DEBUG
    OutputDebugString(s.AsCharPtr());
    #endif
}

//------------------------------------------------------------------------------
/**
*/
void
Win32ConsoleHandler::Confirm(const String& s)
{
    DWORD charsWritten;
    WriteConsole(this->stderrHandle, s.AsCharPtr(), s.Length(), &charsWritten, NULL);
    #ifdef _DEBUG
    OutputDebugString(s.AsCharPtr());
    #endif
    Core::SysFunc::MessageBox(s.AsCharPtr());
}

//------------------------------------------------------------------------------
/**
    Since we are blocking the app waiting for user input, we always provide 
    input.
*/
bool
Win32ConsoleHandler::HasInput()
{
    const DWORD maxNumEvents = 32;
    DWORD numEvents = 0;
    INPUT_RECORD buf[maxNumEvents];
    BOOL success = PeekConsoleInput(this->stdinHandle, buf, maxNumEvents, &numEvents);
    if (success && (numEvents > 0))
    {
        IndexT i;
        for (i = 0; i < (SizeT)numEvents; i++)
        {
            if (buf[i].EventType == KEY_EVENT)
            {
                return true;
            }
        }
        // if no key events where accumuluated we need to clear the
        // buffer to prevent a dead-lock
        FlushConsoleInputBuffer(this->stdinHandle);
    }       
    return false;
}

//------------------------------------------------------------------------------
/**
    Get user input from the console.
*/
String
Win32ConsoleHandler::GetInput()
{
    String result;
    const DWORD bufNumChars = 4096;
    DWORD numCharsRead = 0;
    TCHAR buf[bufNumChars];
    BOOL success = ReadConsole(this->stdinHandle, buf, bufNumChars, &numCharsRead, NULL);
    if (success && (numCharsRead > 0))
    {
        result.Set(buf, numCharsRead);
        result.TrimRight("\n\r");
    }
    return result;
}

}; // namespace IO