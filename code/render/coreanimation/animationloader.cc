//------------------------------------------------------------------------------
//  streamanimationcache.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "coreanimation/animationloader.h"
#include "coreanimation/animationresource.h"
#include "system/byteorder.h"
#include "coreanimation/naxfileformatstructs.h"

namespace CoreAnimation
{
__ImplementClass(CoreAnimation::AnimationLoader, 'SANL', Resources::ResourceLoader);

using namespace IO;
using namespace Util;
using namespace System;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
Resources::ResourceLoader::ResourceInitOutput
AnimationLoader::InitializeResource(const ResourceLoadJob& job, const Ptr<IO::Stream>& stream)
{
    Ptr<AnimKeyBuffer> keyBuffer = nullptr;
    Resources::ResourceLoader::ResourceInitOutput ret;
    
    // map buffer
    uchar* ptr = (uchar*)stream->Map();

    // read header
    Nax3Header* naxHeader = (Nax3Header*)ptr;
    ptr += sizeof(Nax3Header);

    // check magic value
    if (FourCC(naxHeader->magic) != NEBULA_NAX3_MAGICNUMBER)
    {
        n_error("StreamAnimationLoader::InitializeResource(): '%s' has invalid file format (magic number doesn't match)!", stream->GetURI().AsString().AsCharPtr());
        return ret;
    }

    // load animation if it has clips in it
    Util::FixedArray<CoreAnimation::AnimationId> animations(naxHeader->numAnimations);
    animations.Fill(InvalidAnimationId);
    for (IndexT animationIndex = 0; animationIndex < naxHeader->numAnimations; animationIndex++)
    {
        Nax3Anim* anim = (Nax3Anim*)ptr;
        ptr += sizeof(Nax3Anim);

        Util::HashTable<Util::StringAtom, IndexT, 32> clipIndices;
        Util::FixedArray<AnimCurve> curves;
        if (anim->numCurves > 0)
        {
            curves.SetSize(anim->numCurves);
            for (IndexT curveIndex = 0; curveIndex < anim->numCurves; curveIndex++)
            {
                Nax3Curve* naxCurve = (Nax3Curve*)ptr;
                ptr += sizeof(Nax3Curve);

                AnimCurve& curve = curves[curveIndex];
                curve.firstIntervalOffset = naxCurve->firstIntervalOffset;
                curve.numIntervals = naxCurve->numIntervals;
                curve.preInfinityType = (CoreAnimation::InfinityType::Code)naxCurve->preInfinityType;
                curve.postInfinityType = (CoreAnimation::InfinityType::Code)naxCurve->postInfinityType;
                curve.curveType = (CoreAnimation::CurveType::Code)naxCurve->curveType;
            }
        }

        Util::FixedArray<AnimEvent> events;
        if (anim->numEvents > 0)
        {
            events.SetSize(anim->numEvents);
            for (IndexT eventIndex = 0; eventIndex < anim->numEvents; eventIndex++)
            {
                Nax3AnimEvent* naxEvent = (Nax3AnimEvent*)ptr;
                ptr += sizeof(Nax3AnimEvent);

                AnimEvent& event = events[eventIndex];
                event.name = naxEvent->name;
                event.category = naxEvent->category;
                event.time = naxEvent->keyIndex;
            }
        }

        Util::FixedArray<AnimClip> clips;
        if (anim->numClips > 0)
        {
            // setup animation clips
            clips.SetSize(anim->numClips);
            for (IndexT clipIndex = 0; clipIndex < anim->numClips; clipIndex++)
            {
                Nax3Clip* naxClip = (Nax3Clip*)ptr;
                ptr += sizeof(Nax3Clip);

                // setup anim clip object
                AnimClip& clip = clips[clipIndex];
                clip.SetName(naxClip->name);
                clip.firstCurve = naxClip->firstCurve;
                clip.numCurves = naxClip->numCurves;
                clip.firstEvent = naxClip->firstEvent;
                clip.numEvents = naxClip->numEvents;
                clip.firstVelocityCurve = naxClip->firstVelocityCurve;
                clip.numVelocityCurves = naxClip->numVelocityCurves;
                clip.duration = naxClip->duration;
                clip.SetName(naxClip->name);

                for (IndexT i = 0; i < naxClip->numEvents; i++)
                    clip.eventIndexMap.Add(events[naxClip->firstEvent + i].name, naxClip->firstEvent + i);
                clipIndices.Add(naxClip->name, clipIndex);
            }
        }

        // Load keys
        keyBuffer = AnimKeyBuffer::Create();
        keyBuffer->Setup(anim->numIntervals, anim->numKeys, ptr, ptr + sizeof(Nax3Interval) * anim->numIntervals);

        // Advance pointer by keys and timings
        ptr += anim->numKeys * sizeof(float) + anim->numIntervals * sizeof(AnimKeyBuffer::Interval);

        // Create animation
        AnimationCreateInfo info;
        info.clips = clips;
        info.curves = curves;
        info.events = events;
        info.indices = clipIndices;
        info.keyBuffer = keyBuffer;
        AnimationId animid = CreateAnimation(info);
        animations[animationIndex] = animid;
    }

    // unmap memory
    stream->Unmap();

    auto id = animationResourceAllocator.Alloc();
    animationResourceAllocator.Set<0>(id, animations);
    ret.id = id;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
void
AnimationLoader::Unload(const Resources::ResourceId id)
{
    DestroyAnimationResource(id);
}

} // namespace CoreAnimation
