#pragma once
//------------------------------------------------------------------------------
/**
    @class Threading::ThreadBarrier
    
    Block until all thread have arrived at the barrier.
    
    @copyright
    (C) 2009 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__)
#include "threading/win32/win32threadbarrier.h"
namespace Threading
{
class ThreadBarrier : public Win32::Win32ThreadBarrier
{ };
}
#elif (__linux__ || __OSX__ || __APPLE__)
namespace Threading
{
#include "threading/posix/posixthreadbarrier.h"
class ThreadBarrier : public Posix::PosixThreadBarrier
{ };
}
#else
#error "Threading::ThreadBarrier not implemented on this platform!"
#endif


    