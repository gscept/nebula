#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AnimBuilderClip
    
    An animation curve for the N3 AnimBuilder class.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "util/stringatom.h"
#include "coreanimation/infinitytype.h"
#include "coreanimation/animevent.h"
#include "timing/time.h"
#include "model/animutil/animbuildercurve.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class AnimBuilderClip
{
public:
    /// constructor
    AnimBuilderClip();

    /// set name of clip
    void SetName(const Util::StringAtom& n);
    /// get name of clip
    const Util::StringAtom& GetName() const;

    uint firstCurveOffset;
    uint numCurves;
    uint firstEventOffset;
    uint numEvents;
    uint firstVelocityCurveOffset;
    uint numVelocityCurves;

    Timing::Tick duration;

private:
    Util::StringAtom name;
};

//------------------------------------------------------------------------------
/**
*/
inline void
AnimBuilderClip::SetName(const Util::StringAtom& n)
{
    this->name = n;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::StringAtom&
AnimBuilderClip::GetName() const
{
    return this->name;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    