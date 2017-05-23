//------------------------------------------------------------------------------
//  darwintimer.cc
//  (C) 2006 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "timing/darwin/darwintimer.h"

#include <mach/mach.h>
#include <mach/mach_time.h>

namespace Darwin
{

//------------------------------------------------------------------------------
/**
*/
DarwinTimer::DarwinTimer() :
    running(false),
    diffTime(0),
    stopTime(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
    Start the timer. This will update the diffTime member to reflect
    the accumulated time when the timer was not running (basically the
    difference between this timer's time and the real system time).
*/
void
DarwinTimer::Start()
{
    n_assert(!this->running);
    
    // query the current real time and update the diffTime member
    // to take the "lost" time into account since the timer was stopped
    uint64_t curRealTime = mach_absolute_time();
    this->diffTime += curRealTime - this->stopTime;
    this->stopTime = 0;
    this->running = true;
}

//------------------------------------------------------------------------------
/**
    Stop the timer. This will record the current realtime, so that
    the next Start() can measure the time lost between Stop() and Start()
    which must be taken into account to keep track of the difference between
    this timer's time and realtime.
*/
void
DarwinTimer::Stop()
{
    n_assert(this->running);
    this->stopTime = mach_absolute_time();
    this->running = false;
}

//------------------------------------------------------------------------------
/**
    Reset the timer so that will start counting at zero again.
*/
void
DarwinTimer::Reset()
{
    bool wasRunning = this->running;
    if (wasRunning)
    {
        this->Stop();
    }
    this->stopTime = 0;
    this->diffTime = 0;
    if (wasRunning)
    {
        this->Start();
    }
}

//------------------------------------------------------------------------------
/**
    Returns true if the timer is currently running.
*/
bool
DarwinTimer::Running() const
{
    return this->running;
}

//------------------------------------------------------------------------------
/**
    This returns the internal local time as large integer.
*/
uint64_t
DarwinTimer::InternalTime() const
{
    // get the current real time
    uint64_t time;
    if (this->running)
    {
        // we are running, query current time
        time = mach_absolute_time();
    }
    else
    {
        // we are stopped, use time at last stop
        time = this->stopTime;
    }
 
    // convert to local time
    time -= this->diffTime;

    return time;
}

//------------------------------------------------------------------------------
/**
    This returns the timer's current time in seconds.
*/
Timing::Time
DarwinTimer::GetTime() const
{
    // get the current real time
    uint64_t time = this->InternalTime();

    // query the performance frequency
    int64_t freq;
#if 0
    QueryPerformanceFrequency((LARGE_INTEGER*) &freq);
#endif
 
    // convert to seconds
    Timing::Time seconds = ((Timing::Time)time) / ((Timing::Time)freq);
    return seconds;
}

//------------------------------------------------------------------------------
/**
    This returns the timer's current time in "ticks".
*/
uint
DarwinTimer::GetTicks() const
{
    // get the current real time
    uint64_t time = this->InternalTime();

    // query the performance frequency
    int64_t freq;
#if 0
    QueryPerformanceFrequency((LARGE_INTEGER*) &freq);
#endif

    uint64_t ticks64 = time / (freq / 100000);
    return (uint) ticks64;
}

} // namespace Darwin
