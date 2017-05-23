//------------------------------------------------------------------------------
//	linuxthreadlocalptr.cc
//	(C) 2011 A.Weissflog
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "linuxthreadlocalptr.h"
#include "core/sysfunc.h"

namespace Linux
{
//------------------------------------------------------------------------------
/**
 */
LinuxThreadLocalPtr::LinuxThreadLocalPtr()
{
	SysFunc::Setup();
	this->slot = LinuxThreadLocalData::RegisterSlot();
}

} // namespace Linux
