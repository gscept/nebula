//------------------------------------------------------------------------------
//  streamanimationpool.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coreanimation/streamanimationpool.h"
#include "coreanimation/animresource.h"
#include "system/byteorder.h"
#include "coreanimation/naxfileformatstructs.h"

namespace CoreAnimation
{
__ImplementClass(CoreAnimation::StreamAnimationPool, 'SANL', Resources::ResourceStreamPool);

using namespace IO;
using namespace Util;
using namespace System;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
const Util::FixedArray<AnimClip>&
StreamAnimationPool::GetClips(const AnimResourceId id)
{
    return this->Get<0>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
const AnimClip&
StreamAnimationPool::GetClip(const AnimResourceId id, const IndexT index)
{
    return this->Get<0>(id.resourceId)[index];
}

//------------------------------------------------------------------------------
/**
*/
const IndexT StreamAnimationPool::GetClipIndex(const AnimResourceId id, const Util::StringAtom& name)
{
    return this->Get<1>(id.resourceId)[name];
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<AnimKeyBuffer>&
StreamAnimationPool::GetKeyBuffer(const AnimResourceId id)
{
    return this->Get<2>(id.resourceId);
}

//------------------------------------------------------------------------------
/**
*/
Resources::ResourcePool::LoadStatus
StreamAnimationPool::LoadFromStream(const Resources::ResourceId id, const Util::StringAtom& tag, const Ptr<IO::Stream>& stream, bool immediate)
{
    Util::FixedArray<AnimClip>& clips = this->Get<0>(id.resourceId);
    Util::HashTable<Util::StringAtom, IndexT, 32>& clipIndices = this->Get<1>(id.resourceId);
    Ptr<AnimKeyBuffer>& keyBuffer = this->Get<2>(id.resourceId);
    
    // map buffer
    uchar* ptr = (uchar*)stream->Map();

    // read header
    Nax3Header* naxHeader = (Nax3Header*)ptr;
    ptr += sizeof(Nax3Header);

    // check magic value
    if (FourCC(naxHeader->magic) != NEBULA_NAX3_MAGICNUMBER)
    {
        n_error("StreamAnimationLoader::LoadFromStream(): '%s' has invalid file format (magic number doesn't match)!", stream->GetURI().AsString().AsCharPtr());
        return Failed;
    }

    // load animation if it has clips in it
    if (naxHeader->numClips > 0)
    {
        // setup animation clips
        clips.SetSize(naxHeader->numClips);
        IndexT clipIndex;
        SizeT numClips = (SizeT)naxHeader->numClips;
        for (clipIndex = 0; clipIndex < numClips; clipIndex++)
        {
            Nax3Clip* naxClip = (Nax3Clip*)ptr;
            ptr += sizeof(Nax3Clip);

            // setup anim clip object
            AnimClip& clip = clips[clipIndex];
            clip.SetNumCurves(naxClip->numCurves);
            clip.SetStartKeyIndex(naxClip->startKeyIndex);
            clip.SetNumKeys(naxClip->numKeys);
            clip.SetKeyStride(naxClip->keyStride);
            clip.SetKeyDuration(naxClip->keyDuration);
            clip.SetPreInfinityType((InfinityType::Code)naxClip->preInfinityType);
            clip.SetPostInfinityType((InfinityType::Code)naxClip->postInfinityType);
            clip.SetName(naxClip->name);

            // add anim events
            clip.BeginEvents(naxClip->numEvents);
            IndexT eventIndex;
            for (eventIndex = 0; eventIndex < naxClip->numEvents; eventIndex++)
            {
                Nax3AnimEvent* naxEvent = (Nax3AnimEvent*)ptr;
                ptr += sizeof(Nax3AnimEvent);
                AnimEvent animEvent(naxEvent->name, naxEvent->category, naxEvent->keyIndex * clip.GetKeyDuration());
                clip.AddEvent(animEvent);
            }
            clip.EndEvents();

            // setup anim curves
            IndexT curveIndex;
            for (curveIndex = 0; curveIndex < naxClip->numCurves; curveIndex++)
            {
                Nax3Curve* naxCurve = (Nax3Curve*)ptr;
                ptr += sizeof(Nax3Curve);

                AnimCurve& animCurve = clip.CurveByIndex(curveIndex);
                animCurve.SetFirstKeyIndex(naxCurve->firstKeyIndex);
                animCurve.SetActive(naxCurve->isActive != 0);
                animCurve.SetStatic(naxCurve->isStatic != 0);
                animCurve.SetCurveType((CurveType::Code)naxCurve->curveType);
                animCurve.SetStaticKey(vec4(naxCurve->staticKeyX, naxCurve->staticKeyY, naxCurve->staticKeyZ, naxCurve->staticKeyW));
            }
        }

        clipIndices.Clear();
        clipIndices.BeginBulkAdd();
        for (clipIndex = 0; clipIndex < clips.Size(); clipIndex++)
        {
            const AnimClip& curClip = clips[clipIndex];
            n_assert(curClip.GetName().IsValid());
            n_assert(curClip.GetNumCurves() > 0);
            clipIndices.Add(curClip.GetName(), clipIndex);
        }
        clipIndices.EndBulkAdd();

        // precompute the key-slice values in the clips
        for (clipIndex = 0; clipIndex < clips.Size(); clipIndex++)
        {
            clips[clipIndex].PrecomputeKeySliceValues();
        }

        // load keys
        keyBuffer = AnimKeyBuffer::Create();
        keyBuffer->Setup(naxHeader->numKeys);
        void* keyPtr = keyBuffer->Map();
        Memory::Copy(ptr, keyPtr, keyBuffer->GetByteSize());
        keyBuffer->Unmap();
    }

    // unmap memory
    stream->Unmap();
    return Success;
}

//------------------------------------------------------------------------------
/**
*/
void
StreamAnimationPool::Unload(const Resources::ResourceId id)
{
    Util::FixedArray<AnimClip>& clips = this->Get<0>(id.resourceId);
    Util::HashTable<Util::StringAtom, IndexT, 32>& clipIndices = this->Get<1>(id.resourceId);
    Ptr<AnimKeyBuffer>& keyBuffer = this->Get<2>(id.resourceId);

    if (keyBuffer.isvalid())
    {
        keyBuffer->Discard();
        keyBuffer = nullptr;
    }
    clips.Clear();
    clipIndices.Clear();

    this->states[id.poolId] = Resources::Resource::State::Unloaded;
}

} // namespace CoreAnimation
