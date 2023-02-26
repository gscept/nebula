#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::AnimCurve
    
    An animation curve describes a set of animation keys in an AnimKeyBuffer.
    AnimCurves are always part of an AnimClip object, and share properties
    with all other AnimCurves in their AnimClip object. An AnimCurve may
    be collapsed into a single key, so that AnimCurves where all keys
    are identical don't take up any space in the animation key buffer.
    For performance reasons, AnimCurve's are not as flexible as their
    Maya counterparts, for instance it is not possible to set 
    the pre- and post-infinity types per curve, but only per clip.
    
    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "coreanimation/curvetype.h"
#include "coreanimation/infinitytype.h"
#include "math/vec4.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{
class AnimCurve
{
public:
    /// constructor
    AnimCurve();

    uint firstIntervalOffset;
    uint numIntervals;
    CoreAnimation::InfinityType::Code preInfinityType;
    CoreAnimation::InfinityType::Code postInfinityType;
    CurveType::Code curveType;
};

//------------------------------------------------------------------------------
/**
*/
inline
AnimCurve::AnimCurve()
    : firstIntervalOffset(0)
    , numIntervals(0)
    , preInfinityType(CoreAnimation::InfinityType::InvalidInfinityType)
    , postInfinityType(CoreAnimation::InfinityType::InvalidInfinityType)
    , curveType(CoreAnimation::CurveType::InvalidCurveType)
{
    // empty
}

} // namespace AnimCurve
//------------------------------------------------------------------------------
