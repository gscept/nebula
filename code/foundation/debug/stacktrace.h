#pragma once
//------------------------------------------------------------------------------
/**
    @class Debug::StackTrace

    Helper class for generating stack traces

    @copyright
    (C) 2015-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"

#if __WIN32__
#include "debug/win32/win32stacktrace.h"
namespace Debug
{
    class StackTrace : public Win32::Win32StackTrace

    { };
}
#elif ( __OSX__ || __APPLE__ || __linux__ )
#include "debug/posix/posixstacktrace.h"
namespace Debug
{
    class StackTrace : public Posix::PosixStackTrace
    { };
}
#endif
//------------------------------------------------------------------------------
