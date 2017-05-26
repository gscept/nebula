#pragma once
//------------------------------------------------------------------------------
/** 
    @class Animation::PlayClipJob
    
    An AnimJob which simply plays an animation clip.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "animation/animjob.h"

//------------------------------------------------------------------------------
namespace Animation
{
class PlayClipJob : public AnimJob
{
    __DeclareClass(PlayClipJob);
public:
    /// constructor
    PlayClipJob();
    /// destructor
    virtual ~PlayClipJob();   

    /// set the anim clip name to play
    void SetClipName(const Util::StringAtom& clipName);
    /// get the anim clip name
    const Util::StringAtom& GetClipName() const;
    /// set the loop count (overrides duration)
    void SetLoopCount(float loopCount);
    /// get the loop count
    float GetLoopCount() const;

private:
    /// called when attached to anim sequencer
    virtual void OnAttachedToSequencer(const AnimSequencer& animSequencer);
    /// create evaluation job for asynchronous evaluation
    virtual Ptr<Jobs::Job> CreateEvaluationJob(Timing::Tick time, const Ptr<CoreAnimation::AnimSampleBuffer>& mixIn, const Ptr<CoreAnimation::AnimSampleBuffer>& result);
    /// emit anim events inside given time range
    virtual Util::Array<AnimEventInfo> EmitAnimEvents(Timing::Tick startTime, Timing::Tick endTime, const Util::String& optionalCatgeory);

    Util::StringAtom clipName;
    IndexT clipIndex;
    float loopCount;
    bool firstAnimEventCheck;
};

//------------------------------------------------------------------------------
/**
*/
inline void
PlayClipJob::SetClipName(const Util::StringAtom& n)
{
    this->clipName = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
PlayClipJob::GetClipName() const
{
    return this->clipName;
}

//------------------------------------------------------------------------------
/**
*/
inline void
PlayClipJob::SetLoopCount(float c)
{
    this->loopCount = c;
}

//------------------------------------------------------------------------------
/**
*/
inline float
PlayClipJob::GetLoopCount() const
{
    return this->loopCount;
}

} // namespace Animation
//------------------------------------------------------------------------------

