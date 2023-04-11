#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AnimBuilder
    
    Utility class to create and manipulate animation data.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "model/animutil/animbuilderclip.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class AnimBuilder
{
public:
    /// constructor
    AnimBuilder();
    
    /// clear the anim builder
    void Clear();
    /// reserve clip array
    void Reserve(SizeT numClips);

    /// add an animation clips
    void AddClip(const AnimBuilderClip& clip);
    /// get number of animation clips
    SizeT GetNumClips() const;
    /// get clip by index
    AnimBuilderClip& GetClipAtIndex(IndexT i) const;

    /// count the number of curves in all clips
    SizeT CountCurves() const;
    /// count the number of events in all clips
    SizeT CountEvents() const;
    /// count the number of actual keys in all clips
    SizeT CountKeys() const;

    /// Generate velocity curves from translation curves
    void BuildVelocityCurves(float keysPerMS);

    Util::Array<float> keys;
    Util::Array<Timing::Tick> keyTimes;
    Util::Array<AnimBuilderCurve> curves;
    Util::Array<CoreAnimation::AnimEvent> events;
    Util::Array<AnimBuilderClip> clips;

private:
};

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AnimBuilder::GetNumClips() const
{
    return this->clips.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline AnimBuilderClip&
AnimBuilder::GetClipAtIndex(IndexT i) const
{
    return this->clips[i];
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
