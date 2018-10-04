#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::AnimUtil
    
    A class which contains utility methods for animation sampling and mixing.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "coreanimation/animclip.h"
#include "coreanimation/sampletype.h"
#include "coreanimation/animresource.h"
#include "coreanimation/animsamplebuffer.h"
#include "characters/characterjointmask.h"
#include "jobs/job.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{
class AnimUtil
{
public:
    /// OBSOLETE: sample an animation clip at some point in time into an AnimSampleBuffer
    static void Sample(const AnimResourceId& animResource, IndexT clipIndex, SampleType::Code sampleType, Timing::Tick time, float timeFactor, const Ptr<AnimSampleBuffer>& result);
    /// setup a job object which performs sampling
    static Ptr<Jobs::Job> CreateSampleJob(const AnimResourceId& animResource, IndexT clipIndex, SampleType::Code sampleType, Timing::Tick time, float timeFactor, const Characters::CharacterJointMask* mask, const Ptr<AnimSampleBuffer>& result);
    /// setup a job which performs both sampling and mixing
	static Ptr<Jobs::Job> CreateSampleAndMixJob(const AnimResourceId& animResource, IndexT clipIndex, SampleType::Code sampleType, Timing::Tick time, float timeFactor, const Characters::CharacterJointMask* mask, float mixWeight, const Ptr<AnimSampleBuffer>& mixIn, const Ptr<AnimSampleBuffer>& result);
    /// clamp key index into valid range
    static IndexT ClampKeyIndex(IndexT keyIndex, const AnimClip& clip);
    /// compute inbetween ticks for a given sample time
    static Timing::Tick InbetweenTicks(Timing::Tick sampleTime, const AnimClip& clip);
};

} // namespace CoreAnimation
//------------------------------------------------------------------------------