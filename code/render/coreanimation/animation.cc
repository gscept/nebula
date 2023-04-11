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
    animAllocator.Set<Anim_Curves>(id, info.curves);
    animAllocator.Set<Anim_Events>(id, info.events);
    animAllocator.Set<Anim_ClipIndices>(id, info.indices);
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
    animAllocator.Get<Anim_Curves>(id.id24).Clear();
    animAllocator.Get<Anim_Events>(id.id24).Clear();
    animAllocator.Get<Anim_ClipIndices>(id.id24).Clear();
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
const Util::FixedArray<AnimCurve>&
AnimGetCurves(const AnimationId& id)
{
    return animAllocator.Get<Anim_Curves>(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
const IndexT
AnimGetIndex(const AnimationId& id, const Util::StringAtom& name)
{
    return animAllocator.Get<Anim_ClipIndices>(id.id24)[name];
}

} // namespace CoreAnimation
