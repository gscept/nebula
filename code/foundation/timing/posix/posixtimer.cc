//------------------------------------------------------------------------------
//  posixtimer.cc
//  (C) 2007 Oleg Khryptul (Haron)
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "timing/posix/posixtimer.h"
#include <time.h>

namespace Posix
{

//------------------------------------------------------------------------------
/**
*/
PosixTimer::PosixTimer() :
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
PosixTimer::Start()
{
    n_assert(!this->running);

    // query the current real time and update the diffTime member
    // to take the "lost" time into account since the timer was stopped
    timespec times;
    n_assert(clock_gettime(CLOCK_MONOTONIC,&times) == 0);
    
    Timing::Time curRealTime;
    curRealTime = ToTime(times);
    
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
PosixTimer::Stop()
{
    n_assert(this->running);
    timespec times;
    n_assert(clock_gettime(CLOCK_MONOTONIC,&times) == 0);
    this->stopTime = ToTime(times);
    this->running = false;
}

//------------------------------------------------------------------------------
/**
    Reset the timer so that will start counting at zero again.
*/
void
PosixTimer::Reset()
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
PosixTimer::Running() const
{
    return this->running;
}

//------------------------------------------------------------------------------
/**
    Convert timespec to Timing::Time
*/
Timing::Time 
PosixTimer::ToTime(const timespec & ts) const
{
    return (double)ts.tv_sec + (double)ts.tv_nsec * 0.000000001;
} 

//------------------------------------------------------------------------------
/**
    This returns the internal local time as large integer.
*/
Timing::Time
PosixTimer::InternalTime() const
{
    // get the current real time    
    Timing::Time ret;
    if (this->running)
    {
        // we are running, query current time
        timespec tp;
        n_assert(clock_gettime(CLOCK_MONOTONIC,&tp) == 0);
        ret = ToTime(tp);
    }
    else
    {
        // we are stopped, use time at last stop
        ret = this->stopTime;
    }

    // convert to local time
    ret -= this->diffTime;

    return ret;
}

//------------------------------------------------------------------------------
/**
    This returns the timer's current time in seconds.
*/
Timing::Time
PosixTimer::GetTime() const
{
    Timing::Time seconds = this->InternalTime();
    return seconds;
}

//------------------------------------------------------------------------------
/**
    This returns the timer's current time in "ticks".
*/
Timing::Tick
PosixTimer::GetTicks() const
{
    return Timing::SecondsToTicks(this->InternalTime());   
}

} // namespace Posix

