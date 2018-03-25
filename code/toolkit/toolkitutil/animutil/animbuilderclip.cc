//------------------------------------------------------------------------------
//  animbuilderclip.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitutil/animutil/animbuilderclip.h"

namespace ToolkitUtil
{
using namespace CoreAnimation;

//------------------------------------------------------------------------------
/**
*/
AnimBuilderClip::AnimBuilderClip() :
    startKeyIndex(InvalidIndex),
    numKeys(0),
    keyStride(0),
    keyDuration(0),
    preInfinityType(InfinityType::InvalidInfinityType),
    postInfinityType(InfinityType::InvalidInfinityType)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
AnimBuilderClip::ReserveCurves(SizeT numCurves)
{
    this->curveArray.Reserve(numCurves);
}

//------------------------------------------------------------------------------
/**
*/
void
AnimBuilderClip::AddCurve(const AnimBuilderCurve& curve)
{
    this->curveArray.Append(curve);
}

//------------------------------------------------------------------------------
/**
*/
void
AnimBuilderClip::InsertCurve(IndexT index, const AnimBuilderCurve& curve)
{
    this->curveArray.Insert(index, curve);
}

//------------------------------------------------------------------------------
/**
*/
void
AnimBuilderClip::AddEvent(const AnimEvent& event)
{
    this->eventArray.Append(event);
}

} // namespace ToolkitUtil