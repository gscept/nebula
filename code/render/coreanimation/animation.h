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

ID_24_8_TYPE(AnimationId);

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

struct AnimationCreateInfo
{
    Util::FixedArray<AnimClip> clips;
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
/// Get anim clip index
const IndexT AnimGetIndex(const AnimationId& id, const Util::StringAtom& name);
/// Compute key slice pointer and memory size
void AnimComputeSlice(const AnimationId& id, IndexT clipIndex, IndexT keyIndex, SizeT& outSliceByteSize, const Math::vec4*& ptr);

enum
{
    Anim_Clips,
    Anim_KeyIndices,
    Anim_KeyBuffer
};

typedef Ids::IdAllocator<
    Util::FixedArray<AnimClip>,
    Util::HashTable<Util::StringAtom, IndexT, 32>,
    Ptr<AnimKeyBuffer>
> AnimAllocator;
extern AnimAllocator animAllocator;


} // namespace CoreAnimation
//------------------------------------------------------------------------------
