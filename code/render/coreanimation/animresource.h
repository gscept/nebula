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
#include "coreanimation/animclip.h"
#include "coreanimation/animkeybuffer.h"
#include "coreanimation/animsamplemask.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{

RESOURCE_ID_TYPE(AnimResourceId)

class StreamAnimationCache;
extern StreamAnimationCache* animPool;

//------------------------------------------------------------------------------
/**
*/
extern void AnimSampleStep(const AnimCurve* curves,
    int numCurves,
    const Math::vec4& velocityScale,
    const Math::vec4* src0SamplePtr,
    Math::vec4* outSamplePtr,
    uchar* outSampleCounts);

//------------------------------------------------------------------------------
/**
*/
extern void AnimSampleLinear(const AnimCurve* curves,
    int numCurves,
    float sampleWeight,
    const Math::vec4& velocityScale,
    const Math::vec4* src0SamplePtr,
    const Math::vec4* src1SamplePtr,
    Math::vec4* outSamplePtr,
    uchar* outSampleCounts);

//------------------------------------------------------------------------------
/**
*/
extern void AnimMix(const AnimCurve* curves,
    int numCurves,
    const AnimSampleMask* mask,
    float mixWeight,
    const Math::vec4* src0SamplePtr,
    const Math::vec4* src1SamplePtr,
    const uchar* src0SampleCounts,
    const uchar* src1SampleCounts,
    Math::vec4* outSamplePtr,
    uchar* outSampleCounts);

/// create animation resource
const AnimResourceId CreateAnimation(const ResourceCreateInfo& info);
/// destroy animation resource
void DestroyAnimation(const AnimResourceId id);

/// get clips
const Util::FixedArray<AnimClip>& AnimGetClips(const AnimResourceId& id);
/// get single clip
const AnimClip& AnimGetClip(const AnimResourceId& id, const IndexT index);
/// compute key slice pointer and memory size
void AnimComputeSlice(const AnimResourceId& id, IndexT clipIndex, IndexT keyIndex, SizeT& outSliceByteSize, const Math::vec4*& ptr);

} // namespace CoreAnimation
//------------------------------------------------------------------------------
