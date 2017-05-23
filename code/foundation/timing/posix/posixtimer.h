#pragma once
#ifndef POSIX_POSIXTIMER_H
#define POSIX_POSIXTIMER_H
//------------------------------------------------------------------------------
/**
    @class Posix::PosixTimer

    Posix implementation of the Time::Timer class. Under Posix, time
    measurement uses the QueryPerformanceCounter() methods.

    @todo solve multiprocessor issues of QueryPerformanceCounter()
    (different processors may return different PerformanceFrequency
    values, thus, threads should be prevented from switching between
    processors with thread affinities).

    (C) 2006 Radon Labs GmbH
*/
#include "core/types.h"
#include "timing/time.h"

//------------------------------------------------------------------------------
namespace Posix
{
class PosixTimer
{
public:
    /// constructor
    PosixTimer();
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
    /// return internal time
    Timing::Time InternalTime() const;
    
    Timing::Time ToTime(const timespec & ts) const;

    bool running;
    Timing::Time diffTime;  // accumulated time when the timer was not running
    Timing::Time stopTime;  // when was the timer last stopped?
};

} // namespace Posix
//------------------------------------------------------------------------------
#endif

