#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AnimBuilderCurve
    
    An animation curve object in the AnimBuilder class.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "coreanimation/curvetype.h"
#include "coreanimation/infinitytype.h"
#include "util/fixedarray.h"
#include "math/vec4.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class AnimBuilderCurve
{
public:
    /// constructor
    AnimBuilderCurve();
    
    uint firstKeyOffset;        // Offset to key (float value)
    uint firstTimeOffset;
    uint numKeys;               // Number of keys, either vec3 for scale/pos, or vec4 for rot
    CoreAnimation::CurveType::Code curveType;
    CoreAnimation::InfinityType::Code preInfinityType;
    CoreAnimation::InfinityType::Code postInfinityType;
};

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    