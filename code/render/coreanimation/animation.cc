//------------------------------------------------------------------------------
//  animresource.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coreanimation/animation.h"

namespace CoreAnimation
{

AnimAllocator animAllocator;
//------------------------------------------------------------------------------
/**
*/
const AnimationId
CreateAnimation(const AnimationCreateInfo& info)
{
    Ids::Id32 id = animAllocator.Alloc();
    animAllocator.Set<Anim_Clips>(id, info.clips);
    animAllocator.Set<Anim_KeyIndices>(id, info.indices);
    animAllocator.Set<Anim_KeyBuffer>(id, info.keyBuffer);

    AnimationId ret;
    ret.id24 = id;
    ret.id8 = CoreGraphics::AnimResourceIdType;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void DestroyAnimation(const AnimationId id)
{
    animAllocator.Get<Anim_Clips>(id.id24).Clear();
    animAllocator.Get<Anim_KeyIndices>(id.id24).Clear();
    animAllocator.Get<Anim_KeyBuffer>(id.id24)->Discard();
    animAllocator.Set<Anim_KeyBuffer>(id.id24, nullptr);
    animAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<AnimClip>&
AnimGetClips(const AnimationId& id)
{
    return animAllocator.Get<Anim_Clips>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const AnimClip& 
AnimGetClip(const AnimationId& id, const IndexT index)
{
    return animAllocator.Get<Anim_Clips>(id.id24)[index];
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<AnimKeyBuffer>&
AnimGetBuffer(const AnimationId& id)
{
    return animAllocator.Get<Anim_KeyBuffer>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
AnimGetIndex(const AnimationId& id, const Util::StringAtom& name)
{
    return animAllocator.Get<Anim_KeyIndices>(id.id24)[name];
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimComputeSlice(const AnimationId& id, IndexT clipIndex, IndexT keyIndex, SizeT& outSliceByteSize, const Math::vec4*& ptr)
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
