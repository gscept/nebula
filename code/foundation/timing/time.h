#pragma once
//------------------------------------------------------------------------------
/**
    @file timing/time.h
  
    Typedefs for the Timing subsystem
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file	
*/    
#include "core/types.h"

//------------------------------------------------------------------------------
namespace Timing
{
/// the time datatype
typedef double Time;
/// the tick datatype (one tick == 1 millisecond)
typedef int Tick;

//------------------------------------------------------------------------------
/**
    Convert ticks to seconds.
*/
inline Time
TicksToSeconds(Tick ticks)
{
    return ticks * 0.001;
}

//------------------------------------------------------------------------------
/**
    Convert seconds to ticks
*/
inline Tick
SecondsToTicks(Time t)
{
    // perform correct rounding
    return Tick((t * 1000.0) + 0.5);
}

//------------------------------------------------------------------------------
/**
    Put current thread to sleep for specified amount of seconds.
*/
inline void
Sleep(Time t)
{
    n_sleep(t);
}

};
//------------------------------------------------------------------------------
