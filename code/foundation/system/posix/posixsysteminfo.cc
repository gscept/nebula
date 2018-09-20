//------------------------------------------------------------------------------
//  posixsysteminfo.cc
//  (C) 2013 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "system/posix/posixsysteminfo.h"

namespace Posix
{

//------------------------------------------------------------------------------
/**
*/
PosixSystemInfo::PosixSystemInfo()
{
    this->platform = Linux;
#ifdef __x86_64
    this->cpuType = X86_64;
#else
    this->cpuType = X86_32;
#endif
    // XXX: this->numCpuCores = sysInfo.dwNumberOfProcessors;
    // XXX: this->pageSize = sysInfo.dwPageSize;
}

} // namespace Posix
