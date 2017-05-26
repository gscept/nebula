#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreAnimation::AnimResource
  
    A AnimResource is a collection of related animation clips (for instance 
    all animation clips of a character). AnimResources contain read-only
    data and are usually shared between several clients. One AnimResource
    usually contains the data of one animation resource file.
    
    (C) 2008 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "resources/resource.h"
#include "coreanimation/animclip.h"
#include "coreanimation/animkeybuffer.h"

//------------------------------------------------------------------------------
namespace CoreAnimation
{
class AnimResource : public Resources::Resource
{
    __DeclareClass(AnimResource);
public:
    /// constructor
    AnimResource();
    /// destructor
    virtual ~AnimResource();

    /// unload the resource, or cancel the pending load
    virtual void Unload();

    /// get number of animation clips
    SizeT GetNumClips() const;
    /// get animation clip at index
    const AnimClip& GetClipByIndex(IndexT clipIndex) const;
    /// return true if clip exists by name
    bool HasClip(const Util::StringAtom& clipName) const;
    /// get clip index by name, returns invalid index if not found
    IndexT GetClipIndexByName(const Util::StringAtom& clipName) const;
    /// get clip by name
    const AnimClip& GetClipByName(const Util::StringAtom& clipName) const;
    /// get pointer to AnimKeyBuffer
    const Ptr<AnimKeyBuffer>& GetKeyBuffer() const;

    /// get pointer to start of a key slice, and return size of a key slice
    const Math::float4* ComputeKeySlicePointerAndSize(IndexT clipIndex, IndexT keyIndex, SizeT& outSliceByteSize) const;

private:
    friend class StreamAnimationLoader;

    /// setup the object, called by the resource loader
    const Ptr<AnimKeyBuffer>& SetupKeyBuffer(SizeT numKeysInBuffer);
    /// begin setting up clips, called by the resource loader
    void BeginSetupClips(SizeT numClips);
    /// access to anim clip for setup
    AnimClip& Clip(IndexT clipIndex);
    /// finish setting up clips, called by the resource loader
    void EndSetupClips();
    /// (re-)build the clip-index-map (maps names to clip indices
    void BuildClipIndexMap();

    Util::FixedArray<AnimClip> animClips;
    Util::Dictionary<Util::StringAtom, IndexT> clipIndexMap;
    Ptr<AnimKeyBuffer> animKeyBuffer;
};

//------------------------------------------------------------------------------
/**
*/
inline SizeT
AnimResource::GetNumClips() const
{
    return this->animClips.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline const AnimClip&
AnimResource::GetClipByIndex(IndexT i) const
{
    return this->animClips[i];
}

//------------------------------------------------------------------------------
/**
*/
inline const Ptr<AnimKeyBuffer>&
AnimResource::GetKeyBuffer() const
{
    return this->animKeyBuffer;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
AnimResource::HasClip(const Util::StringAtom& clipName) const
{
    return this->clipIndexMap.Contains(clipName);
}

//------------------------------------------------------------------------------
/**
*/
inline IndexT
AnimResource::GetClipIndexByName(const Util::StringAtom& clipName) const
{
    IndexT index = this->clipIndexMap.FindIndex(clipName);
    if (InvalidIndex != index)
    {
        return this->clipIndexMap.ValueAtIndex(index);
    }
    else
    {
        return InvalidIndex;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline const AnimClip&
AnimResource::GetClipByName(const Util::StringAtom& clipName) const
{
    return this->animClips[this->clipIndexMap[clipName]];
}

} // namespace CoreAnimation
//------------------------------------------------------------------------------
