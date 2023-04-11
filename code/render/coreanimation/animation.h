#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::AnimResource
  
    A AnimResource is a collection of related animation clips (for instance 
    all animation clips of a character). AnimResources contain read-only
    data and are usually shared between several clients. One AnimResource
    usually contains the data of one animation resource file.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "resources/resource.h"
#include "ids/idallocator.h"
#include "coreanimation/animclip.h"
#include "coreanimation/animkeybuffer.h"
#include "coreanimation/animsamplemask.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{

ID_24_8_TYPE(AnimationId);

extern AnimKeyBuffer::Interval
FindNextInterval(
    const AnimCurve& curve
    , const Timing::Tick time
    , uint& key
    , const AnimKeyBuffer::Interval* sampleTimes
);

//------------------------------------------------------------------------------
/**
*/
extern void AnimSampleStep(
    const AnimClip& clip,
    const Util::FixedArray<AnimCurve>& curves,
    const Timing::Tick time,
    const Math::vec4& velocityScale,
    const Util::FixedArray<Math::vec4>& idleSamples,
    const float* srcSamplePtr,
    const AnimKeyBuffer::Interval* intervalPtr,
    uint* outSampleKeyPtr,
    float* outSamplePtr,
    uchar* outSampleCounts
);

//------------------------------------------------------------------------------
/**
*/
extern void AnimSampleLinear(
    const AnimClip& clip,
    const Util::FixedArray<AnimCurve>& curves,
    const Timing::Tick time,
    const Math::vec4& velocityScale,
    const Util::FixedArray<Math::vec4>& idleSamples,
    const float* srcSamplePtr,
    const AnimKeyBuffer::Interval* intervalPtr,
    uint* outSampleKeyPtr,
    float* outSamplePtr,
    uchar* outSampleCounts
);

//------------------------------------------------------------------------------
/**
*/
extern void AnimMix(
    const AnimClip& clip,
    const SizeT numSamples,
    const AnimSampleMask* mask,
    float mixWeight,
    const float* src0SamplePtr,
    const float* src1SamplePtr,
    const uchar* src0SampleCounts,
    const uchar* src1SampleCounts,
    float* outSamplePtr,
    uchar* outSampleCounts
);

struct AnimationCreateInfo
{
    Util::FixedArray<AnimClip> clips;
    Util::FixedArray<AnimCurve> curves;
    Util::FixedArray<AnimEvent> events;
    Util::HashTable<Util::StringAtom, IndexT, 32> indices;
    Ptr<AnimKeyBuffer> keyBuffer;
};

/// Create animation resource
const AnimationId CreateAnimation(const AnimationCreateInfo& info);
/// Destroy animation resource
void DestroyAnimation(const AnimationId id);

/// Get clips
const Util::FixedArray<AnimClip>& AnimGetClips(const AnimationId& id);
/// Get single clip
const AnimClip& AnimGetClip(const AnimationId& id, const IndexT index);
/// Get anim buffer
const Ptr<AnimKeyBuffer>& AnimGetBuffer(const AnimationId& id);
/// Get curves
const Util::FixedArray<AnimCurve>& AnimGetCurves(const AnimationId& id);
/// Get anim clip index
const IndexT AnimGetIndex(const AnimationId& id, const Util::StringAtom& name);

enum
{
    Anim_Clips,
    Anim_Curves,
    Anim_Events,
    Anim_ClipIndices,
    Anim_KeyBuffer
};

typedef Ids::IdAllocator<
    Util::FixedArray<AnimClip>,
    Util::FixedArray<AnimCurve>,
    Util::FixedArray<AnimEvent>,
    Util::HashTable<Util::StringAtom, IndexT, 32>,
    Ptr<AnimKeyBuffer>
> AnimAllocator;
extern AnimAllocator animAllocator;


} // namespace CoreAnimation
//------------------------------------------------------------------------------
