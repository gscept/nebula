#pragma once
//------------------------------------------------------------------------------
/**
    @class System::SystemInfo
    
    Provides information about the host system.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
namespace System
{
enum PlatformType
{
    Win32,
    Linux,

    UnknownPlatform,
};

enum CpuArchType
{
    X86_32,             // any 32-bit x86
    X86_64,             // any 64-bit x86

    UnknownCpuType,
};

/// convert platform to string
inline const char* 
PlatformTypeAsString(PlatformType p)
{
    switch (p)
    {
        case Win32:     return "win32";
        case Linux:     return "linux";
        default:        return "unknownplatform";
    }
}

/// convert CpuType to string
inline const char*
CpuArchTypeAsString(CpuArchType cpu)
{
    switch (cpu)
    {
        case X86_32:            return "x86_32";
        case X86_64:            return "x86_64";
        default:                return "unknowncputype";
    }
}

extern CpuArchType CpuArch;
extern PlatformType Platform;
extern SizeT NumCpuCores;
extern SizeT PageSize;

} /// namespace System
#if __WIN32__
#include "system/win32/win32systeminfo.h"
namespace System
{
class SystemInfo : public Win32::Win32SystemInfo {};
}
#elif __linux__ || __APPLE__
#include "system/posix/posixsysteminfo.h"
namespace System
{
class SystemInfo : public Posix::PosixSystemInfo {};
}
#else
#error "System::SystemInfo not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
