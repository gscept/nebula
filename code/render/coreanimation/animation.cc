//------------------------------------------------------------------------------
//  animresource.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

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
    animAllocator.Set<Anim_Curves>(id, info.curves);
    animAllocator.Set<Anim_Events>(id, info.events);
    animAllocator.Set<Anim_ClipIndices>(id, info.indices);
    animAllocator.Set<Anim_KeyBuffer>(id, info.keyBuffer);

    AnimationId ret = id;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void DestroyAnimation(const AnimationId id)
{
    animAllocator.Get<Anim_Clips>(id.id).Clear();
    animAllocator.Get<Anim_Curves>(id.id).Clear();
    animAllocator.Get<Anim_Events>(id.id).Clear();
    animAllocator.Get<Anim_ClipIndices>(id.id).Clear();
    animAllocator.Get<Anim_KeyBuffer>(id.id)->Discard();
    animAllocator.Set<Anim_KeyBuffer>(id.id, nullptr);
    animAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<AnimClip>&
AnimGetClips(const AnimationId& id)
{
    return animAllocator.Get<Anim_Clips>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const AnimClip& 
AnimGetClip(const AnimationId& id, const IndexT index)
{
    return animAllocator.Get<Anim_Clips>(id.id)[index];
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<AnimKeyBuffer>&
AnimGetBuffer(const AnimationId& id)
{
    return animAllocator.Get<Anim_KeyBuffer>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<AnimCurve>&
AnimGetCurves(const AnimationId& id)
{
    return animAllocator.Get<Anim_Curves>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
AnimGetIndex(const AnimationId& id, const Util::StringAtom& name)
{
    return animAllocator.Get<Anim_ClipIndices>(id.id)[name];
}

} // namespace CoreAnimation
