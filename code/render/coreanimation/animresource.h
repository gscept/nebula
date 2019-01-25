#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::AnimResource
  
    A AnimResource is a collection of related animation clips (for instance 
    all animation clips of a character). AnimResources contain read-only
    data and are usually shared between several clients. One AnimResource
    usually contains the data of one animation resource file.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "resources/resource.h"
#include "coreanimation/animclip.h"
#include "coreanimation/animkeybuffer.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{

RESOURCE_ID_TYPE(AnimResourceId)

class StreamAnimationPool;
extern StreamAnimationPool* animPool;

/// create animation resource
const AnimResourceId CreateAnimation(const ResourceCreateInfo& info);
/// destroy animation resource
void DestroyAnimation(const AnimResourceId id);

/// get clips
const Util::FixedArray<AnimClip>& AnimGetClips(const AnimResourceId& id);
/// get single clip
const AnimClip& AnimGetClip(const AnimResourceId& id, const IndexT index);
/// compute key slice pointer and memory size
void AnimComputeSlice(const AnimResourceId& id, IndexT clipIndex, IndexT keyIndex, SizeT& outSliceByteSize, const Math::float4*& ptr);

} // namespace CoreAnimation
//------------------------------------------------------------------------------
