#pragma once
//------------------------------------------------------------------------------
/**
    @class Threading::ThreadBarrier
    
    Block until all thread have arrived at the barrier.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__ || __XBOX360__)
#include "threading/win360/win360threadbarrier.h"
namespace Threading
{
class ThreadBarrier : public Win360::Win360ThreadBarrier
{ };
}
#elif __PS3__
#include "threading/ps3/ps3threadbarrier.h"
namespace Threading
{
class ThreadBarrier : public PS3::PS3ThreadBarrier
{ };
}
#elif __WII__
#include "threading/wii/wiithreadbarrier.h"
namespace Threading
{
class ThreadBarrier : public Wii::WiiThreadBarrier
{ };
}
#elif (linux || __OSX__ || __APPLE__)
namespace Threading
{
#include "threading/posix/posixthreadbarrier.h"
class ThreadBarrier : public Posix::PosixThreadBarrier
{ };
}
#else
#error "Threading::ThreadBarrier not implemented on this platform!"
#endif


    