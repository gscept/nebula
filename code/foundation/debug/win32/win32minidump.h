#pragma once
#ifndef WIN32_WIN32MINIDUMP_H
#define WIN32_WIN32MINIDUMP_H
//------------------------------------------------------------------------------
/**
    @class Win32::Win32MiniDump
  
    Win32 implementation of MiniDump.
    
    @copyright
    (C) 2007 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/string.h"

//------------------------------------------------------------------------------
namespace Win32
{
class Win32MiniDump
{
public:
    /// setup the the Win32 exception callback hook
    static void Setup();
    /// write a mini dump
    static bool WriteMiniDump();

private:
    /// internal mini-dump-writer method with extra exception info
    static bool WriteMiniDumpInternal(EXCEPTION_POINTERS* exceptionInfo);
    /// build a filename for the dump file
    static Util::String BuildMiniDumpFilename();
    /// the actual exception handler function called back by Windows
    static LONG WINAPI ExceptionCallback(EXCEPTION_POINTERS* exceptionInfo);
};
 
} // namespace Win32
//------------------------------------------------------------------------------
#endif
