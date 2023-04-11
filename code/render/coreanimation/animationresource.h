#pragma once
//------------------------------------------------------------------------------
/**
    An animation resource holds a set of animations from a loaded NAX file

    @copyright
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "resources/resourceid.h"
#include "ids/idallocator.h"
#include "util/fixedarray.h"
#include "animation.h"
namespace CoreAnimation
{

RESOURCE_ID_TYPE(AnimationResourceId);

/// Get animation from resource
const AnimationId AnimationResourceGetAnimation(const AnimationResourceId id, IndexT index);

/// Destroy animation resource
void DestroyAnimationResource(const AnimationResourceId id);

typedef Ids::IdAllocator<
    Util::FixedArray<AnimationId>
> AnimationResourceAllocator;
extern AnimationResourceAllocator animationResourceAllocator;

} // namespace CoreAnimation
