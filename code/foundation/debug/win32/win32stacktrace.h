#pragma once
//------------------------------------------------------------------------------
/**
    @class Win32::Win32Stacktrace

    Win32 implementation of stacktrace

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
    /// create a stack trace string
    static const Util::Array<Util::String> GenerateStackTrace();
    static void PrintStackTrace(int skip = 3);
};
}

