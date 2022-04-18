#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::AnimBuilder
    
    Utility class to create and manipulate animation data.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "toolkitutil/animutil/animbuilderclip.h"

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

    /// generate velocity curves from translation curves
    void BuildVelocityCurves();
    /// fix first-key-indices in anim curves
    void FixAnimCurveFirstKeyIndices();
    /// fix the static key values for inactive curves
    void FixInactiveCurveStaticKeyValues();
    /// fix keys which might have had invalid values
    void FixInvalidKeyValues();
    /// cut keys from end of tracks
    void TrimEnd(SizeT numKeys);

    /// downsample all anim curves once
    //void Downsample();
    /// downsample a specific clip
    //void DownsampleClip(IndexT i);
    /// optimize the animation data
    //void Optimize();
    /// fix key offsets (call after optimizing)
    //void FixKeyOffsets() const;
    /// build additional data for anim driven motion
    //void BuildAnimDrivenMotionData();
    
private:
    Util::Array<AnimBuilderClip> clipArray;
};

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AnimBuilder::GetNumClips() const
{
    return this->clipArray.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline AnimBuilderClip&
AnimBuilder::GetClipAtIndex(IndexT i) const
{
    return this->clipArray[i];
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
