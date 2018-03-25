//------------------------------------------------------------------------------
/**   
    (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "foundation/stdneb.h"
#include "threading/win360/win360criticalsection.h"     
#include "system/systeminfo.h"
   
namespace Win360
{
#if NEBULA3_USER_CRITICAL_SECTION
//------------------------------------------------------------------------------
/**
*/  
Win360CriticalSection::Win360CriticalSection():
    lockerThread(0),
    spinMax(4096),
    semaphore(NULL),
    waiterCount(0),
    recursiveLockCount(0)
{
    if (Core::SysFunc::GetSystemInfo()->GetNumCpuCores() == 1) spinMax = 0;
}     

//------------------------------------------------------------------------------
/**
*/ 
Win360CriticalSection::~Win360CriticalSection()
{
    if (this->semaphore)
    {
        CloseHandle(this->semaphore);
    }
}
#endif  

}
//------------------------------------------------------------------------------
