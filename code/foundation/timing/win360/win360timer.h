#pragma once
#ifndef WIN360_WIN360TIMER_H
#define WIN360_WIN360TIMER_H
//------------------------------------------------------------------------------
/**
    @class Win360::Win360Timer
    
    Win32/Xbox360 implementation of the Time::Timer class. Uses 
    the QueryPerformanceCounter() functions.

    @todo solve multiprocessor issues of QueryPerformanceCounter()
    (different processors may return different PerformanceFrequency
    values, thus, threads should be prevented from switching between
    processors with thread affinities).
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "timing/time.h"

//------------------------------------------------------------------------------
namespace Win360
{
class Win360Timer
{
public:
    /// constructor
    Win360Timer();
    /// start/continue the timer
    void Start();
    /// stop the timer
    void Stop();
    /// reset the timer
    void Reset();
    /// return true if currently running
    bool Running() const;
    /// get current time in seconds
    Timing::Time GetTime() const;
    /// get current time in ticks
    Timing::Tick GetTicks() const;

private:
    /// return internal time as 64 bit integer
    __int64 InternalTime() const;

    bool running;
    __int64 diffTime;  // accumulated time when the timer was not running
    __int64 stopTime;  // when was the timer last stopped?
};

} // namespace Win360
//------------------------------------------------------------------------------
#endif
   