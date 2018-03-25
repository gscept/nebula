//------------------------------------------------------------------------------
//  framesynctimer.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "framesync/framesynctimer.h"

namespace FrameSync
{
__ImplementClass(FrameSync::FrameSyncTimer, 'FSYT', Core::RefCounted);
__ImplementSingleton(FrameSync::FrameSyncTimer);

using namespace Timing;

//------------------------------------------------------------------------------
/**
*/
FrameSyncTimer::FrameSyncTimer() :
    time(0.0),
    ticks(0),
    frameTime(0.0),
    frameTicks(0),
    scaledTime(0.0),
    timeFactor(1.0),
	frameIndex(0),
    isValid(false)   
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
FrameSyncTimer::~FrameSyncTimer()
{
    if (this->IsValid())
    {
        this->Discard();
    }
    __DestructSingleton;
}

//------------------------------------------------------------------------------
/**
    Query master time from the FrameSyncHandlerThread once at 
    creation time. This is safe but slow, but necessary to immediately
    provide a valid time stamp in the first frame.
*/
void
FrameSyncTimer::Setup()
{
    n_assert(!this->isValid);
    this->isValid = true;

    // do a slow first-time init
    this->UpdateTimePolling();

    // fix frame times to a reasonable value
    this->frameTime = 1.0 / 60.0;
    this->frameTicks = Timing::SecondsToTicks(this->frameTime);

	// start main timer
	this->masterTimer.Start();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSyncTimer::Discard()
{
    n_assert(this->isValid);
    this->isValid = false;
}

//------------------------------------------------------------------------------
/**
    Update the time by polling the FrameSyncHandlerThread singleton.
    This requires a CriticalSection to be taken and should not be used
    for threads which run in lockstep mode (i.e. the game thread or the
    render thread) since these will update their time automatically
    during the frame-sync-point.
*/
void
FrameSyncTimer::UpdateTimePolling()
{
	this->Update(this->masterTimer.GetTime());
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSyncTimer::StartTime()
{
	this->masterTimer.Start();	
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSyncTimer::StopTime()
{
	this->masterTimer.Stop();
}

//------------------------------------------------------------------------------
/**
*/
void
FrameSyncTimer::ResetTime()
{
	this->masterTimer.Reset();
}

//------------------------------------------------------------------------------
/**
*/
bool
FrameSyncTimer::IsTimeRunning() const
{
	return this->masterTimer.Running();
}

//------------------------------------------------------------------------------
/**
    This method will be called once per frame at the frame-sync-point
    of the FrameSyncHandlerThread. Note that this method will always be 
    called from the local thread, never from another thread, thus we don't
    need to care about thread-safety.
*/
void
FrameSyncTimer::Update(Time newTime)
{
    this->frameTime = newTime - this->time; 
    this->scaledTime = this->scaledTime + this->frameTime * this->timeFactor;    
    if (this->frameTime < 0.001)
    {
        this->frameTime = 0.001;
    }
    this->time = newTime;

    Tick newTicks = Timing::SecondsToTicks(newTime);
    this->frameTicks = newTicks - this->ticks;
    if (this->frameTicks < 1)
    {
        this->frameTicks = 1;
    }
    this->ticks = newTicks;

	this->frameIndex++;
}

//------------------------------------------------------------------------------
/**
*/
Timing::Time 
FrameSyncTimer::GetScaledFrameTime() const
{
    return this->frameTime * this->timeFactor;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
FrameSyncTimer::GetFrameIndex() const
{
	return this->frameIndex;
}

} // namespace FrameSync