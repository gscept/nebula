#pragma once
//------------------------------------------------------------------------------
/**
    @class Timing::Timer
  
    A timer object is the most basic object for time measurement. More
    advanced timing classes often build on top of Timer.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/config.h"
#if __WIN32__
#include "timing/win32/win32timer.h"
namespace Timing
{
class Timer : public Win32::Win32Timer
{ };
}
#elif __linux__
#include "timing/posix/posixtimer.h"
namespace Timing
{
class Timer : public Posix::PosixTimer
{ };
}
#else
#error "Timing::Timer not implemented on this platform!"
#endif

