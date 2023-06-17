//------------------------------------------------------------------------------
//  posixsysteminfo.cc
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "system/systeminfo.h"
#include "system/posix/posixsysteminfo.h"
#include <thread>
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
    System::CpuArch = System::X86_64;
#else
    System::CpuArch = System::X86_32;
#endif
    System::PageSize = getpagesize();

    System::NumCpuCores = std::thread::hardware_concurrency();
    // XXX: this->numCpuCores = sysInfo.dwNumberOfProcessors;
    // XXX: this->pageSize = sysInfo.dwPageSize;
}

} // namespace Posix
