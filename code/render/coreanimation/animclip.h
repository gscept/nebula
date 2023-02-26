#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::AnimClip
  
    An animation clip is a collection of related animation curves (for
    instance all curves required to animate a character).

    @copyright
    (C) 2008 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "coreanimation/infinitytype.h"
#include "coreanimation/animcurve.h"
#include "coreanimation/animevent.h"
#include "util/stringatom.h"
#include "timing/time.h"
#include "util/dictionary.h"
#include "util/fixedarray.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{
class AnimClip
{
public:
    /// constructor
    AnimClip();

    /// set the name of the clip
    void SetName(const Util::StringAtom& n);
    /// get the name of the clip
    const Util::StringAtom& GetName() const;

    Util::StringAtom name;

    SizeT numCurves;
    IndexT firstCurve;
    SizeT numEvents;
    IndexT firstEvent;
    SizeT numVelocityCurves;
    IndexT firstVelocityCurve;

    Timing::Tick duration;
    Util::Dictionary<Util::StringAtom, IndexT> eventIndexMap;
};

//------------------------------------------------------------------------------
/**
*/
inline void
AnimClip::SetName(const Util::StringAtom& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
AnimClip::GetName() const
{
    return this->name;
}

} // namespace CoreAnimation
//------------------------------------------------------------------------------
