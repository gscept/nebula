#pragma once
//------------------------------------------------------------------------------
/**
    @class Posix::PosixStackTrace

    Posix implementation of stacktrace. Function, file and line come from
    addr2line, so they need binutils installed and debug info in the module.

    @copyright
    (C) 2026 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Posix
{
class PosixStackTrace
{
public:
    /// create a stack trace, one line per frame starting at the caller, also written straight to stderr
    static const Util::Array<Util::String> GenerateStackTrace();
    /// print the stack trace with n_printf, leaving out the caller's first skip frames
    static void PrintStackTrace(int skip = 0);
};
}
