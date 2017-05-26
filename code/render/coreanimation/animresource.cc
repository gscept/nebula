//------------------------------------------------------------------------------
//  animresource.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coreanimation/animresource.h"

namespace CoreAnimation
{
__ImplementClass(AnimResource, 'ANRS', Resources::Resource);

using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
AnimResource::AnimResource()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
AnimResource::~AnimResource()
{
    // make sure we've been unloaded
    n_assert(!this->animKeyBuffer.isvalid());
}

//------------------------------------------------------------------------------
/**
    This method is called by the resource subsystem when the resource
    should unload its data.
*/
void
AnimResource::Unload()
{
    if (this->animKeyBuffer.isvalid())
    {
        this->animKeyBuffer->Discard();
        this->animKeyBuffer = 0;
    }
    this->animClips.Clear();
    this->clipIndexMap.Clear();
    Resource::Unload();
}

//------------------------------------------------------------------------------
/**
    This method is called by the attached resource loader to setup
    the animation key buffer.
*/
const Ptr<AnimKeyBuffer>&
AnimResource::SetupKeyBuffer(SizeT numKeysInBuffer)
{
    n_assert(!this->animKeyBuffer.isvalid());
    this->animKeyBuffer = AnimKeyBuffer::Create();
    this->animKeyBuffer->Setup(numKeysInBuffer);
    return this->animKeyBuffer;
}

//------------------------------------------------------------------------------
/**
    Begin setting up animation clips. This method is called by the
    attached resource loader.
*/
void
AnimResource::BeginSetupClips(SizeT numClips)
{
    n_assert(this->animClips.IsEmpty());
    n_assert(this->clipIndexMap.IsEmpty());
    this->animClips.SetSize(numClips);
}

//------------------------------------------------------------------------------
/**
    Gain access to an anim clip object during setup. This method is
    called by the resource loader. This mechanism prevents excessive
    copying and allocation/deallocation of temporary objects.
*/
AnimClip&
AnimResource::Clip(IndexT clipIndex)
{
    return this->animClips[clipIndex];
}

//------------------------------------------------------------------------------
/**
    Finish setting up animation clips. This method is called by the attached
    resource loader. The method will setup the clip index map for fast
    lookup-by-name of animation clips.
*/
void
AnimResource::EndSetupClips()
{
    n_assert(this->animClips.Size() > 0);

    // make sure that all anim clips have been setup, and setup the
    // name lookup table
    this->BuildClipIndexMap();

    // precompute the key-slice values in the clips
    IndexT clipIndex;
    for (clipIndex = 0; clipIndex < this->animClips.Size(); clipIndex++)
    {
        this->animClips[clipIndex].PrecomputeKeySliceValues();
    }
}

//------------------------------------------------------------------------------
/**
    Re-builds the clip index map which maps clip names to clip indices.
*/
void
AnimResource::BuildClipIndexMap()
{
    this->clipIndexMap.Clear();
    this->clipIndexMap.Reserve(this->animClips.Size());
    this->clipIndexMap.BeginBulkAdd();
    IndexT clipIndex;
    for (clipIndex = 0; clipIndex < this->animClips.Size(); clipIndex++)
    {
        const AnimClip& curClip = this->animClips[clipIndex];
        n_assert(curClip.GetName().IsValid());
        n_assert(curClip.GetNumCurves() > 0);
        this->clipIndexMap.Add(curClip.GetName(), clipIndex);
    }
    this->clipIndexMap.EndBulkAdd();
}

//------------------------------------------------------------------------------
/**
    Compute start pointer and size of a key slice of a clip in the
    anim resource. NOTE: if all anim curves of the clip are static (which
    means there are exist no actual keys), then the method will return
    a NULL pointer, and an outSliceByteSlice of 0.
*/
const float4*
AnimResource::ComputeKeySlicePointerAndSize(IndexT clipIndex, IndexT keyIndex, SizeT& outSliceByteSize) const
{
    const AnimClip& clip = this->animClips[clipIndex];
    n_assert(clip.AreKeySliceValuesValid());
    IndexT firstKeyIndex = clip.GetKeySliceFirstKeyIndex();
    if (InvalidIndex == firstKeyIndex)
    {
        // all curves of the clip are static
        outSliceByteSize = 0;
        return 0;
    }
    else
    {
        outSliceByteSize = clip.GetKeySliceByteSize();
        IndexT sliceKeyIndex = firstKeyIndex + keyIndex * clip.GetKeyStride();
        const float4* sliceKeyPtr = this->animKeyBuffer->GetKeyBufferPointer() + sliceKeyIndex;
        return sliceKeyPtr;
    }
}

} // namespace CoreAnimation
