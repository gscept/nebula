#pragma once
//------------------------------------------------------------------------------
/**
    @class FrameSync::FrameSyncTimer
    
    A thread-local time source object which is synchronized with the
    sync point in the FrameSyncHandlerThread. Time values are thread-locally
    cached and thus no thread-synchronization is necessary when reading
    time values. FrameSyncTimer objects are updated inside the 
    FrameSyncHandler::ArriveAtSyncPoint() method.

    Threads interested in the master time create a FrameSyncTimer singleton
    and register it with FrameSyncHandlerThread through the GraphicsInterface
    singleton.

    Please note that when calling the Start()/Stop()/Reset() methods,
    that the ENTIRE master time will be affected (these methods go 
    straight through to the FrameSyncHandler object).
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file	
*/ 
#include "core/refcounted.h"
#include "core/singleton.h"
#include "timing/time.h"
#include "timing/timer.h"

//------------------------------------------------------------------------------
namespace FrameSync
{
class FrameSyncTimer : public Core::RefCounted
{
    __DeclareClass(FrameSyncTimer);
    __DeclareSingleton(FrameSyncTimer);
public:
    /// constructor
    FrameSyncTimer();
    /// destructor
    virtual ~FrameSyncTimer();

    /// setup the object
    void Setup();
    /// discard the object
    void Discard();
    /// return true if object is valid
    bool IsValid() const;

    /// update the time through polling, only necessary for threads other then render/game thread!
    void UpdateTimePolling();
    /// start the master time (DON'T CALL FREQUENTLY!)
    void StartTime();
    /// stop the master time (DON'T CALL FREQUENTLY!)
    void StopTime();
    /// reset the master time (DON'T CALL FREQUENTLY!)
    void ResetTime();
    /// return true if master time is running (DON'T CALL FREQUENTLY!)
    bool IsTimeRunning() const;

    /// get current time
    Timing::Time GetTime() const;
    /// get current time in ticks
    Timing::Tick GetTicks() const;
    /// get current frame time
    Timing::Time GetFrameTime() const;
    /// get current frame time ins ticks
    Timing::Tick GetFrameTicks() const;
    /// get current frame count
    IndexT GetFrameIndex() const;

    /// set time factor
    void SetTimeFactor(Timing::Time factor);
    /// get time factor
    Timing::Time GetTimeFactor() const;
    /// get scaled time
    Timing::Time GetScaledTime() const;
    /// get scaled time
    Timing::Time GetScaledFrameTime() const;

private:
    friend class FrameSyncHandlerThread;

    /// update the time (always called from local thread)
    void Update(Timing::Time masterTime);

	
    Timing::Time time;
    Timing::Tick ticks;
    Timing::Time frameTime;	
    Timing::Tick frameTicks;
    Timing::Time scaledTime;
    Timing::Time timeFactor;
	Timing::Time realTime;
	Timing::Timer masterTimer;
	IndexT frameIndex;
    bool isValid;
};

//------------------------------------------------------------------------------
/**
*/
inline bool
FrameSyncTimer::IsValid() const
{
    return this->isValid;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Time
FrameSyncTimer::GetTime() const
{
    return this->time;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
FrameSyncTimer::GetTicks() const
{
    return this->ticks;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Time
FrameSyncTimer::GetFrameTime() const
{
    return this->frameTime;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
FrameSyncTimer::GetFrameTicks() const
{
    return this->frameTicks;
}
 
//------------------------------------------------------------------------------
/**
*/
inline void 
FrameSyncTimer::SetTimeFactor(Timing::Time factor)
{
    this->timeFactor = factor;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Time 
FrameSyncTimer::GetTimeFactor() const
{
    return this->timeFactor;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Time
FrameSyncTimer::GetScaledTime()const 
{
    return this->scaledTime;
}

} // namespace FrameSync
//------------------------------------------------------------------------------


