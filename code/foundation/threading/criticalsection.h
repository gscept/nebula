#pragma once
//------------------------------------------------------------------------------
/**
    @class Threading::CriticalSection

    Critical section objects are used to protect a portion of code from parallel
    execution. Define a static critical section object and use its Enter() 
    and Leave() methods to protect critical sections of your code.
    
    @copyright
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__)
#include "threading/win32/win32criticalsection.h"
namespace Threading
{
class CriticalSection : public Win32::Win32CriticalSection
{ };
}
#elif __linux__
#include "threading/linux/linuxcriticalsection.h"
namespace Threading
{
class CriticalSection : public Linux::LinuxCriticalSection
{ };
}
#elif ( __OSX__ || __APPLE__ )
#include "threading/posix/posixcriticalsection.h"
namespace Threading
{
class CriticalSection : public Posix::PosixCriticalSection
{ };
}
#else
#error "Threading::CriticalSection not implemented on this platform!"
#endif

namespace Threading
{
struct CriticalScope
{
    CriticalScope(Threading::CriticalSection* section)
        : section(section)
    {
        this->section->Enter();
    }

    ~CriticalScope()
    {
        this->section->Leave();
        this->section = nullptr;
    }

    Threading::CriticalSection* section;
};

}

//------------------------------------------------------------------------------
    
