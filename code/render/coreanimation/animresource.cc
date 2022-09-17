//------------------------------------------------------------------------------
//  animresource.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coreanimation/animresource.h"

namespace CoreAnimation
{

AnimAllocator animAllocator;
//------------------------------------------------------------------------------
/**
*/
const AnimResourceId 
CreateAnimation(const AnimationCreateInfo& info)
{
    Ids::Id32 id = animAllocator.Alloc();
    animAllocator.Set<Anim_Clips>(id, info.clips);
    animAllocator.Set<Anim_KeyIndices>(id, info.indices);
    animAllocator.Set<Anim_KeyBuffer>(id, info.keyBuffer);

    AnimResourceId ret;
    ret.resourceId = id;
    ret.resourceType = CoreGraphics::AnimResourceIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void DestroyAnimation(const AnimResourceId id)
{
    animAllocator.Get<Anim_Clips>(id.resourceId).Clear();
    animAllocator.Get<Anim_KeyIndices>(id.resourceId).Clear();
    animAllocator.Get<Anim_KeyBuffer>(id.resourceId)->Discard();
    animAllocator.Set<Anim_KeyBuffer>(id.resourceId, nullptr);
    animAllocator.Dealloc(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<AnimClip>&
AnimGetClips(const AnimResourceId& id)
{
    return animAllocator.Get<Anim_Clips>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const AnimClip& 
AnimGetClip(const AnimResourceId& id, const IndexT index)
{
    return animAllocator.Get<Anim_Clips>(id.resourceId)[index];
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<AnimKeyBuffer>&
AnimGetBuffer(const AnimResourceId& id)
{
    return animAllocator.Get<Anim_KeyBuffer>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
AnimGetIndex(const AnimResourceId& id, const Util::StringAtom& name)
{
    return animAllocator.Get<Anim_KeyIndices>(id.resourceId)[name];
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimComputeSlice(const AnimResourceId& id, IndexT clipIndex, IndexT keyIndex, SizeT& outSliceByteSize, const Math::vec4*& ptr)
{
    const AnimClip& clip = AnimGetClip(id, clipIndex);
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
        const Ptr<AnimKeyBuffer>& buffer = AnimGetBuffer(id);
        outSliceByteSize = clip.GetKeySliceByteSize();
        IndexT sliceKeyIndex = firstKeyIndex + keyIndex * clip.GetKeyStride();
        ptr = buffer->GetKeyBufferPointer() + sliceKeyIndex;
    }
}

} // namespace CoreAnimation
