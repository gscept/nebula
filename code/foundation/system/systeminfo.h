#pragma once
//------------------------------------------------------------------------------
/**
    @class System::SystemInfo
    
    Provides information about the host system.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#if __WIN32__
#include "system/win32/win32systeminfo.h"
namespace System
{
class SystemInfo : public Win32::Win32SystemInfo {};
}
#elif __XBOX360__
#include "system/xbox360/xbox360systeminfo.h"
namespace System
{
class SystemInfo : public Xbox360::Xbox360SystemInfo {};
}
#elif __WII__
#include "system/wii/wiisysteminfo.h"
namespace System
{
class SystemInfo : public Wii::WiiSystemInfo {};
}
#elif __PS3__
#include "system/ps3/ps3systeminfo.h"
namespace System
{
class SystemInfo : public PS3::PS3SystemInfo {};
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
