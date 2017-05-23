#pragma once
#ifndef DARWIN_DARWINTIMER_H
#define DARWIN_DARWINTIMER_H
//------------------------------------------------------------------------------
/**
    @class Darwin::DarwinTimer
    
    Darwin implementation of the Time::Timer class. Under Darwin, time
    measurement uses the mach_absolute_time() methods.

    (C) 2006 Radon Labs GmbH
*/
#include "core/types.h"
#include "timing/time.h"

//------------------------------------------------------------------------------
namespace Darwin
{
class DarwinTimer
{
public:
    /// constructor
    DarwinTimer();
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
    uint GetTicks() const;

private:
    /// return internal time as 64 bit integer
    uint64_t InternalTime() const;

    bool running;
    uint64_t diffTime;  // accumulated time when the timer was not running
    uint64_t stopTime;  // when was the timer last stopped?
};

} // namespace Darwin
//------------------------------------------------------------------------------
#endif
   
