//------------------------------------------------------------------------------
//  darwinsysteminfo.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "system/darwin/darwinsysteminfo.h"

namespace Darwin
{

//------------------------------------------------------------------------------
/**
*/
DarwinSystemInfo::DarwinSystemInfo()
{
    this->platform = MacOSX;
    this->cpuType = X86_32;
    // XXX: this->numCpuCores = sysInfo.dwNumberOfProcessors;
    // XXX: this->pageSize = sysInfo.dwPageSize;
}

} // namespace Darwin
