//------------------------------------------------------------------------------
//  animclip.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "coreanimation/animclip.h"

namespace CoreAnimation
{
using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
AnimClip::AnimClip()
    : numCurves(0)
    , firstCurve(0)
    , numEvents(0)
    , firstEvent(0)
    , numVelocityCurves(0)
    , firstVelocityCurve(0)
    , duration(0)
{
    // empty
}

} // namespace CoreAnimation
