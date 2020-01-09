//------------------------------------------------------------------------------
// debug.cc
//  (C) 2002 RadonLabs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "core/types.h"
#include "core/sysfunc.h"
#include "io/console.h"
#include "util/string.h"
#include "debugbreak.h"

//------------------------------------------------------------------------------
/**
    This function is called by n_assert() when the assertion fails.
*/
void 
n_barf(const char* exp, const char* file, int line)
{
    if (IO::Console::HasInstance())
    {
        n_error("*** NEBULA ASSERTION ***\nexpression: %s\nfile: %s\nline: %d\n", exp, file, line);
    }
    else
    {
    	Util::String msg;
    	msg.Format("*** NEBULA ASSERTION ***\n%s(%d): expression: %s\n", file, line, exp);
        Core::SysFunc::Error(msg.AsCharPtr());
    }
}

//------------------------------------------------------------------------------
/**
    This function is called by n_assert2() when the assertion fails.
*/
void
n_barf2(const char* exp, const char* msg, const char* file, int line)
{
    if (IO::Console::HasInstance())
    {
        n_error("*** NEBULA ASSERTION ***\nprogrammer says: %s\nexpression: %s\nfile: %s\nline: %d\n", msg, exp, file, line);
    }
    else
    {
    	Util::String fmt;
		fmt.Format("*** NEBULA ASSERTION ***\nprogrammer says: %s\nexpression: %s\nfile: %s\nline: %d\n", msg, exp, file, line);
        Core::SysFunc::Error(fmt.AsCharPtr());
    }
}


//------------------------------------------------------------------------------
/**
*/
void
n_barf_fmt(const char *	exp, const char *fmt, const char *file, int line, ...)
{
	Util::String msg;
	va_list argList;
	va_start(argList, line);
	msg.FormatArgList(fmt, argList);
	va_end(argList);
	Util::String format = Util::String::Sprintf("*** NEBULA ASSERTION ***\nprogrammer says : %s\nfile : %s\nline : %d\nexpression : %s\n", msg.AsCharPtr(), file, line, exp);
	if (IO::Console::HasInstance())
	{
		n_error(format.AsCharPtr());
	}
	else
	{
		Core::SysFunc::Error(format.AsCharPtr());
	}
}

//------------------------------------------------------------------------------
/**
    This function is called when a serious situation is encountered which
    requires abortion of the application.
*/
void __cdecl
n_error(const char* msg, ...)
{
    va_list argList;
    va_start(argList, msg);
    if (IO::Console::HasInstance())
    {
        IO::Console::Instance()->Error(msg, argList);
    }
    else
    {
        Util::String str;
        str.FormatArgList(msg, argList);
        Core::SysFunc::Error(str.AsCharPtr());
    }
    va_end(argList);
}

//------------------------------------------------------------------------------
/**
    This function is called when a warning should be issued which doesn't
    require abortion of the application.
*/
void __cdecl
n_warning(const char* msg, ...)
{
    va_list argList;
    va_start(argList, msg);
    if (IO::Console::HasInstance())
    {
        IO::Console::Instance()->Warning(msg, argList);
    }
    else
    {
        Util::String str;
        str.FormatArgList(msg, argList);
        Core::SysFunc::MessageBox(str.AsCharPtr());
    }
    va_end(argList);
}        

//------------------------------------------------------------------------------
/**
    This function is called when a message should be displayed to the
    user which requires a confirmation (usually displayed as a MessageBox).
*/
void __cdecl
n_confirm(const char* msg, ...)
{
    va_list argList;
    va_start(argList, msg);
    if (IO::Console::HasInstance())
    {
        IO::Console::Instance()->Confirm(msg, argList);
    }
    else
    {
        Util::String str;
        str.FormatArgList(msg, argList);
        Core::SysFunc::MessageBox(str.AsCharPtr());
    }
    va_end(argList);
}

//------------------------------------------------------------------------------
/**
    Nebula's printf replacement. Will redirect the text to the console
    and/or logfile.

     - 27-Nov-98   floh    created
*/
void __cdecl
n_printf(const char *msg, ...)
{
    va_list argList;
    va_start(argList, msg);
    IO::Console::Instance()->Print(msg, argList);
    va_end(argList);
}

//------------------------------------------------------------------------------
/**
    Put the message to the debug window.

    - 26-Mar-05    kims    created
*/
void __cdecl
n_dbgout(const char *msg, ...)
{
    va_list argList;
    va_start(argList,msg);
    IO::Console::Instance()->DebugOut(msg, argList);
    va_end(argList);
}

//------------------------------------------------------------------------------
/**
    Put process to sleep.

     - 21-Dec-98   floh    created
*/
void 
n_sleep(double sec)
{
    Core::SysFunc::Sleep(sec);
}

//------------------------------------------------------------------------------
/**
*/
void
n_break()
{
    debug_break();
}
