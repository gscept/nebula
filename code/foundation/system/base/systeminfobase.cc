//------------------------------------------------------------------------------
//  systeminfobase.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "system/base/systeminfobase.h"

namespace Base
{
using namespace Util;

//------------------------------------------------------------------------------
/**
*/
SystemInfoBase::SystemInfoBase() :
    platform(UnknownPlatform),
    cpuType(UnknownCpuType),
    numCpuCores(0),
    pageSize(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
String
SystemInfoBase::PlatformAsString(Platform p)
{
    switch (p)
    {
        case Win32:     return "win32";
        case Linux:     return "linux";        
        default:        return "unknownplatform";
    }
}

//------------------------------------------------------------------------------
/**
*/
String
SystemInfoBase::CpuTypeAsString(CpuType c)
{
    switch (c)
    {
        case X86_32:            return "x86_32";
        case X86_64:            return "x86_64";
        case PowerPC_Xbox360:   return "powerpc_xbox360";
        case PowerPC_PS3:       return "powerpc_ps3";
        case PowerPC_Wii:       return "powerpc_wii";
        default:                return "unknowncputype";
    }
}

} // namespace Base