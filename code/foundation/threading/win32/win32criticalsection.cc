//------------------------------------------------------------------------------
/**   
    (C) 2009 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "foundation/stdneb.h"
#include "threading/win32/win32criticalsection.h"     
#include "system/systeminfo.h"
   
namespace Win32
{
#if NEBULA_USER_CRITICAL_SECTION
//------------------------------------------------------------------------------
/**
*/  
Win32CriticalSection::Win32CriticalSection():
    lockerThread(0),
    spinMax(4096),
    semaphore(NULL),
    waiterCount(0),
    recursiveLockCount(0)
{
    if (Core::SysFunc::GetSystemInfo()->GetNumCpuCores() == 1)
		spinMax = 0;
}     

//------------------------------------------------------------------------------
/**
*/ 
Win32CriticalSection::~Win32CriticalSection()
{
    if (this->semaphore)
    {
        CloseHandle(this->semaphore);
    }
}
#endif  

}
//------------------------------------------------------------------------------
