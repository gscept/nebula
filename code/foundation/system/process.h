#pragma once
//------------------------------------------------------------------------------
/**
    @class System::AppLauncher
    
    Launch an external application.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __WIN32__
#include "win32/win32process.h"
namespace System
{
typedef Win32::Win32Process Process;
}
#elif __linux__
#include "posix/posixprocess.h"
namespace System
{
typedef Posix::PosixProcess Process;
}
#else
#error "System::Process not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
    
