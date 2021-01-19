#pragma once
//------------------------------------------------------------------------------
/**
    @class System::SystemInfo
    
    Provides information about the host system.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#if __WIN32__
#include "system/win32/win32systeminfo.h"
namespace System
{
class SystemInfo : public Win32::Win32SystemInfo {};
}
#elif __linux__
#include "system/posix/posixsysteminfo.h"
namespace System
{
class SystemInfo : public Posix::PosixSystemInfo {};
}
#else
#error "System::SystemInfo not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
