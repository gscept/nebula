#pragma once
//------------------------------------------------------------------------------
/**
    @class System::Library
    
    Use to load dynamic libraries and their addresses
    
    @copyright
    (C) 2023 Individual contributors, see AUTHORS file
*/
#if __WIN32__
#include "win32/win32library.h"
namespace System
{
typedef Win32::Win32Library Library;
}
#elif (__linux__ || __OSX__ || __APPLE__)
#include "posix/posixlibrary.h"
namespace System
{
typedef Posix::PosixLibrary Library;
}
#else
#error "System::Library not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
    
