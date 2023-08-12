//------------------------------------------------------------------------------
//  @file animationresource.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "animationresource.h"
namespace CoreAnimation
{

AnimationResourceAllocator animationResourceAllocator;

//------------------------------------------------------------------------------
/**
*/
const AnimationId
AnimationResourceGetAnimation(const AnimationResourceId id, IndexT index)
{
    return animationResourceAllocator.Get<0>(id.resourceId)[index];
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyAnimationResource(const AnimationResourceId id)
{
    auto animations = animationResourceAllocator.Get<0>(id.resourceId);
    for (IndexT i = 0; i < animations.Size(); i++)
    {
        DestroyAnimation(animations[i]);
    }
    animationResourceAllocator.Dealloc(id.resourceId);
}

} // namespace CoreAnimation
