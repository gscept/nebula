//------------------------------------------------------------------------------
//  posixsysteminfo.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "system/posix/posixsysteminfo.h"

namespace System
{
    CpuArchType CpuArch;
    PlatformType Platform;
    SizeT NumCpuCores;
    SizeT PageSize;
}


namespace Posix
{

//------------------------------------------------------------------------------
/**
*/
PosixSystemInfo::PosixSystemInfo()
{
    System::Platform = System::Linux;
#ifdef __x86_64
    System::CpuType = System::X86_64;
#else
    System::CpuType = System::X86_32;
#endif
    System::PageSize = getpagesize();

    struct cpu_raw_data_t raw;
    struct cpu_id_t data;
    cpuid_get_raw_data(&raw);
    cpu_identify(&raw, &data);
    System::NumCpuCores = data.num_cores;
    // XXX: this->numCpuCores = sysInfo.dwNumberOfProcessors;
    // XXX: this->pageSize = sysInfo.dwPageSize;
}

} // namespace Posix
