#pragma once
//------------------------------------------------------------------------------
/**
    @class Animation::AnimJob
  
    Descibes a single animation sampling job in the AnimController. AnimJob
    objects have a start time and a duration and are arranged in parallel 
    tracks. The sampling results of parallel AnimJobs at a given point in
    time are mixed into a single resulting animation by the AnimController.
    Subclasses of AnimJob are used to implement specific tasks like
    a lookat-controller, IK, and so forth...

    FIXME: the current implementation of setting an absolute evaluation
    time doesn't allow to manipulate the playback speed (for this,
    advancing the time by a relative amount would be better).

    (C) 2008 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/refcounted.h"
#include "coreanimation/animresource.h"
#include "coreanimation/animsamplebuffer.h"
#include "jobs/jobport.h"
#include "math/vector.h"
#include "math/point.h"
#include "animation/animjobenqueuemode.h"
#include "characters/characterjointmask.h"

//------------------------------------------------------------------------------
namespace Animation
{
class AnimSequencer;
class AnimEventInfo;

class AnimJob : public Core::RefCounted
{
    __DeclareClass(AnimJob);
public:
    /// constructor
    AnimJob();
    /// destructor
    virtual ~AnimJob();

    /// set human readable name (only used for debugging)
    void SetName(const Util::StringAtom& id);
    /// get human readable name (only used for debugging)
    const Util::StringAtom& GetName() const;

    /// return true if the job is currently attached to a sequencer
    bool IsAttachedToSequencer() const;
    /// return true if the job has currently playing (EvalTime within start/end time)
    bool IsActive(Timing::Tick time) const;
    /// return true if the job has been queued for playback but has not started yet
    bool IsPending(Timing::Tick time) const;
    /// return true if anim job is stopping or expired
    bool IsStoppingOrExpired(Timing::Tick time) const;
    /// return true when the job has expired
    bool IsExpired(Timing::Tick time) const;
    /// returns true if the clip is playing, but has been paused
    bool IsPaused() const;

    /// set track index, defines blend priority and relationship to other jobs on same track
    void SetTrackIndex(IndexT trackIndex);
    /// get track index
    IndexT GetTrackIndex() const;
    /// set the enqueue behaviour of the new job (default is intercept)
    void SetEnqueueMode(AnimJobEnqueueMode::Code enqueueMode);
    /// get the enqueue behaviour of the new job
    AnimJobEnqueueMode::Code GetEnqueueMode() const;
    /// exclusive tag (for AnimJobEnqueueMode::IgnoreIfSameExclTagActive)
    void SetExclusiveTag(IndexT id);
    /// exclusive flag set?
    IndexT GetExclusiveTag() const;

    /// set the start time of the anim job (relative to base time)
    void SetStartTime(Timing::Tick time);
    /// get the start time of the anim job (relative to base time)
    Timing::Tick GetStartTime() const;    
    /// set the duration of the anim job (0 == infinite)
    void SetDuration(Timing::Tick time);
    /// get the duration of the anim job
    Timing::Tick GetDuration() const;
    /// return true if the anim job is infinite
    bool IsInfinite() const;
    /// set the fade-in time of the anim job
    void SetFadeInTime(Timing::Tick fadeInTime);
    /// get the fade-in time of the anim job
    Timing::Tick GetFadeInTime() const;
    /// set the fade-out time of the anim job
    void SetFadeOutTime(Timing::Tick fadeOutTime);
    /// get the fade-out time of the anim job
    Timing::Tick GetFadeOutTime() const;
    /// jump to a specific time in the job
    void SetTime(Timing::Tick time);
    /// get current time of anim job
    Timing::Tick GetTime() const;
    /// set time factor
    void SetTimeFactor(float timeFactor);
    /// get time factor
    float GetTimeFactor() const;
    /// set sample time offset (if sampling should not start at the beginning)
    void SetTimeOffset(Timing::Tick timeOffset);
    /// get sample time offset
    Timing::Tick GetTimeOffset() const;  

    /// set blend weight of the anim job (default is 1.0)
    void SetBlendWeight(float w);
    /// get blend weight of the anim job
    float GetBlendWeight() const;

    /// set joint mask
    void SetMask(const Characters::CharacterJointMask* mask);
    /// get joint mask
    const Characters::CharacterJointMask* GetMask() const;

    /// get the absolute start time (BaseTime + StartTime)
    Timing::Tick GetAbsoluteStartTime() const;
    /// get the absolute end time (BaseTime + StartTime + Duration)
    Timing::Tick GetAbsoluteEndTime() const;
    /// get the absolute, computed end time ((BaseTime + StartTime + Duration) - FadeOutTime)
    Timing::Tick GetAbsoluteStopTime() const;

protected:
    friend class AnimSequencer;

    /// set the base time of the anim job (set by sequencer when job is attached)
    void SetBaseTime(Timing::Tick time);
    /// get the base time of the anim job
    Timing::Tick GetBaseTime() const;
    /// called when attached to anim sequencer
    virtual void OnAttachedToSequencer(const AnimSequencer& animSequencer);
    /// called when removed from sequencer
    virtual void OnRemoveFromSequencer();
    /// compute current blend weight, this should take fade-in into account
    float ComputeBlendWeight(Timing::Tick relEvalTime) const;
    /// fix fade-in/fade-out times if the sum is bigger then the play duration
    void FixFadeTimes();
    /// create evaluation job for asynchronous evaluation
    virtual Ptr<Jobs::Job> CreateEvaluationJob(Timing::Tick time, const Ptr<CoreAnimation::AnimSampleBuffer>& mixIn, const Ptr<CoreAnimation::AnimSampleBuffer>& result);
    /// emit anim events inside given time range
    virtual Util::Array<AnimEventInfo> EmitAnimEvents(Timing::Tick startTime, Timing::Tick endTime, const Util::String& optionalCatgeory);
    /// compute sample time for next evaluation, always done, also if character isn't visible and no evaluation takes place
    virtual void UpdateTimes(Timing::Tick time);
    /// stop the anim job at the given time
    virtual void Stop(Timing::Tick time);
    /// pause the anim job at the given time
    virtual void Pause(bool pause);


    const AnimSequencer* animSequencer;
    const Characters::CharacterJointMask* mask;
    Util::StringAtom name;
    AnimJobEnqueueMode::Code enqueueMode;
    IndexT trackIndex;
    IndexT exclusiveTag;
    Timing::Tick baseTime;
    Timing::Tick startTime;
    Timing::Tick duration;
    Timing::Tick fadeInTime;
    Timing::Tick fadeOutTime;
    Timing::Tick curRelEvalTime;
    Timing::Tick lastRelEvalTime;
    Timing::Tick curSampleTime;
    Timing::Tick lastSampleTime;
    Timing::Tick timeOffset;
    float timeFactor;
    float blendWeight;
    bool isPaused;
};

//------------------------------------------------------------------------------
/**
*/
inline void
AnimJob::SetEnqueueMode(AnimJobEnqueueMode::Code m)
{
    this->enqueueMode = m;
}

//------------------------------------------------------------------------------
/**
*/
inline AnimJobEnqueueMode::Code
AnimJob::GetEnqueueMode() const
{
    return this->enqueueMode;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimJob::SetName(const Util::StringAtom& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
AnimJob::GetName() const
{
    return this->name;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimJob::SetExclusiveTag(IndexT i)
{
    this->exclusiveTag = i;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
AnimJob::GetExclusiveTag() const
{
    return this->exclusiveTag;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimJob::SetTrackIndex(IndexT i)
{
    this->trackIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
AnimJob::GetTrackIndex() const
{
    return this->trackIndex;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimJob::SetBaseTime(Timing::Tick t)
{
    this->baseTime = t;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
AnimJob::GetBaseTime() const
{
    return this->baseTime;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimJob::SetStartTime(Timing::Tick t)
{
    this->startTime = t;
    this->lastSampleTime = t;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
AnimJob::GetStartTime() const
{
    return this->startTime;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimJob::SetDuration(Timing::Tick t)
{
    this->duration = t;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
AnimJob::GetDuration() const
{
    return this->duration;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimJob::IsInfinite() const
{
    return (0 == this->duration);
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimJob::SetFadeInTime(Timing::Tick t)
{
    this->fadeInTime = t;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
AnimJob::GetFadeInTime() const
{
    return this->fadeInTime;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimJob::SetFadeOutTime(Timing::Tick t)
{
    this->fadeOutTime = t;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
AnimJob::GetFadeOutTime() const
{
    return this->fadeOutTime;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AnimJob::SetTime( Timing::Tick time )
{
    this->curSampleTime = this->startTime + time;
    this->lastSampleTime = this->curSampleTime;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick 
AnimJob::GetTime() const
{
    return this->curSampleTime;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimJob::SetBlendWeight(float w)
{
    this->blendWeight = w;
}

//------------------------------------------------------------------------------
/**
*/
inline float
AnimJob::GetBlendWeight() const
{
    return this->blendWeight;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimJob::SetMask(const Characters::CharacterJointMask* mask)
{
    this->mask = mask;
}

//------------------------------------------------------------------------------
/**
*/
inline const Characters::CharacterJointMask*
AnimJob::GetMask() const
{
    return this->mask;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
AnimJob::SetTimeFactor(float f)
{
    this->timeFactor = f;
}

//------------------------------------------------------------------------------
/**
*/
inline float
AnimJob::GetTimeFactor() const
{
    return this->timeFactor;
}

//------------------------------------------------------------------------------
/**
*/
inline void
AnimJob::SetTimeOffset(Timing::Tick t)
{
    this->timeOffset = t;
}

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
AnimJob::GetTimeOffset() const
{
    return this->timeOffset;
}   

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimJob::IsPaused() const
{
    return this->isPaused;
}

} // namespace Animation
//------------------------------------------------------------------------------
