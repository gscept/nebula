#pragma once
//------------------------------------------------------------------------------
/**
    @class Timing::Timer
  
    A timer object is the most basic object for time measurement. More
    advanced timing classes often build on top of Timer.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if (__WIN32__ || __XBOX360__)
#include "timing/win360/win360timer.h"
namespace Timing
{
class Timer : public Win360::Win360Timer
{ };
}
#elif __WII__
#include "timing/wii/wiitimer.h"
namespace Timing
{
class Timer : public Wii::WiiTimer
{ };
}
#elif __PS3__
#include "timing/ps3/ps3timer.h"
namespace Timing
{
class Timer : public PS3::PS3Timer
{ };
}
#elif linux
#include "timing/posix/posixtimer.h"
namespace Timing
{
class Timer : public Posix::PosixTimer
{ };
}
#else
#error "Timing::Timer not implemented on this platform!"
#endif

