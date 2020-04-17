//------------------------------------------------------------------------------
//  animresource.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coreanimation/animresource.h"
#include "streamanimationpool.h"

namespace CoreAnimation
{

StreamAnimationPool* animPool = nullptr;

//------------------------------------------------------------------------------
/**
*/
const AnimResourceId 
CreateAnimation(const ResourceCreateInfo& info)
{
	return animPool->CreateResource(info.resource, info.tag, info.successCallback, info.failCallback, !info.async).As<AnimResourceId>();
}

//------------------------------------------------------------------------------
/**
*/
void DestroyAnimation(const AnimResourceId id)
{
	animPool->DiscardResource(id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<AnimClip>&
AnimGetClips(const AnimResourceId& id)
{
	return animPool->GetClips(id);
}

//------------------------------------------------------------------------------
/**
*/
const AnimClip& 
AnimGetClip(const AnimResourceId& id, const IndexT index)
{
	return animPool->GetClip(id, index);
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimComputeSlice(const AnimResourceId& id, IndexT clipIndex, IndexT keyIndex, SizeT& outSliceByteSize, const Math::vec4*& ptr)
{
	const AnimClip& clip = animPool->GetClip(id, clipIndex);
	n_assert(clip.AreKeySliceValuesValid());
	IndexT firstKeyIndex = clip.GetKeySliceFirstKeyIndex();
	if (InvalidIndex == firstKeyIndex)
	{
		// all curves of the clip are static
		outSliceByteSize = 0;
		ptr = nullptr;
	}
	else
	{
		const Ptr<AnimKeyBuffer>& buffer = animPool->GetKeyBuffer(id);
		outSliceByteSize = clip.GetKeySliceByteSize();
		IndexT sliceKeyIndex = firstKeyIndex + keyIndex * clip.GetKeyStride();
		ptr = buffer->GetKeyBufferPointer() + sliceKeyIndex;
	}
}

} // namespace CoreAnimation
