#pragma once
//------------------------------------------------------------------------------
/**
    @class System::NebulaSettings
    
    Platform independent way for storing persistent settings 
    Will use registry in windows and config files on other platforms
        
    (C) 2013 Individual contributors, see AUTHORS file
*/
#if __WIN32__
#include "system/win32/win32registry.h"
namespace System
{
typedef Win32::Win32Registry NebulaSettings;
}
#elif __LINUX__
#include "system/posix/posixsettings.h"
namespace System
{
typedef Posix::PosixSettings NebulaSettings;
}
#else
#error "System::NebulaSettings not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
    