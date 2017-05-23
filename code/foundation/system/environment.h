#pragma once
//------------------------------------------------------------------------------
/**
    @class System::Environment
    
    Provides read-access to environment variables. Useful for tools.
        
    (C) 2013 Individual contributors, see AUTHORS file
*/
#if __WIN32__
#include "system/win32/win32environment.h"
namespace System
{
typedef Win32::Win32Environment Environment;
}
#elif __LINUX__
#include "system/posix/posixenvironment.h"
namespace System
{
typedef Posix::PosixEnvironment Environment;
}
#else
#error "System::Environment not implemented on this platform!"
#endif
//------------------------------------------------------------------------------