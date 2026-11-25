#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::Win32Stacktrace

    Win32 implementation of stacktrace. Function, file and line come from
    dbghelp, so they need the module's pdb.

    @copyright
    (C) 2015-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32StackTrace
{
public:
    /// create a stack trace, one line per frame starting at the caller, also written straight to stderr
    static const Util::Array<Util::String> GenerateStackTrace();
    /// print the stack trace with n_printf, leaving out the caller's first skip frames
    static void PrintStackTrace(int skip = 0);
};
}
