#pragma once
//------------------------------------------------------------------------------
/**
    An anim sample mask controls which samples should be used, and which should be avoided

    (C) 2018-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "util/fixedarray.h"
#include "util/stringatom.h"
namespace CoreAnimation
{

struct AnimSampleMask
{
    Util::StringAtom name;
    Util::FixedArray<Math::scalar> weights;

    /// handle finding character joint masks
    bool operator==(const AnimSampleMask& rhs) const { return name == rhs.name;  };
};

} // namespace CoreAnimation
