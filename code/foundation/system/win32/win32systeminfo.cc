//------------------------------------------------------------------------------
//  win32systeminfo.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "system/win32/win32systeminfo.h"

namespace Win32
{

//------------------------------------------------------------------------------
/**
*/
Win32SystemInfo::Win32SystemInfo()
{
    this->platform = Win32;

    // get runtime-info from Windows
    SYSTEM_INFO sysInfo;
    ::GetSystemInfo(&sysInfo);    
    switch (sysInfo.wProcessorArchitecture)
    {
        case PROCESSOR_ARCHITECTURE_AMD64:  this->cpuType = X86_64; break;
        case PROCESSOR_ARCHITECTURE_INTEL:  this->cpuType = X86_32; break;
        default:                            this->cpuType = UnknownCpuType; break;
    }
    this->numCpuCores = sysInfo.dwNumberOfProcessors;
    this->pageSize = sysInfo.dwPageSize;
}

} // namespace Win32