//------------------------------------------------------------------------------
//  animbuildersaver.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "model/animutil/animbuildersaver.h"
#include "io/ioserver.h"
#include "coreanimation/naxfileformatstructs.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace IO;
using namespace System;
using namespace CoreAnimation;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
bool
AnimBuilderSaver::Save(const URI& uri, const Util::Array<AnimBuilder>& animBuilders, Platform::Code platform)
{
    // make sure the target directory exists
    IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        ByteOrder byteOrder(ByteOrder::Host, Platform::GetPlatformByteOrder(platform));
        AnimBuilderSaver::WriteHeader(stream, animBuilders, byteOrder);
        AnimBuilderSaver::WriteAnimations(stream, animBuilders, byteOrder);

        stream->Close();
        stream = nullptr;
        return true;
    }
    else
    {
        // failed to open write stream
        return false;
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AnimBuilderSaver::WriteHeader(const Ptr<Stream>& stream, const Util::Array<AnimBuilder>& animBuilders, const ByteOrder& byteOrder)
{
    // setup header
    Nax3Header nax3Header;
    nax3Header.magic         = byteOrder.Convert<uint>(NEBULA_NAX3_MAGICNUMBER);
    nax3Header.numAnimations = byteOrder.Convert(animBuilders.Size());

    // write header
    stream->Write(&nax3Header, sizeof(nax3Header));
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimBuilderSaver::WriteAnimations(const Ptr<IO::Stream>& stream, const Util::Array<AnimBuilder>& animBuilders, const System::ByteOrder& byteOrder)
{
    for (auto& anim : animBuilders)
    {
        Nax3Anim nax3;
        nax3.numClips = anim.GetNumClips();
        nax3.numEvents = anim.events.Size();
        nax3.numCurves = anim.curves.Size();
        nax3.numKeys = anim.keys.Size();
        nax3.numIntervals = 0;
        for (const auto& curve : anim.curves)
        {
            nax3.numIntervals += curve.numKeys - 1;
        }

        // write header
        stream->Write(&nax3, sizeof(nax3));

        Util::Array<Nax3Interval> intervals;

        for (const auto& curve : anim.curves)
        {
            // write curve attributes
            Nax3Curve nax3Curve;
            nax3Curve.firstIntervalOffset = intervals.Size();
            nax3Curve.numIntervals = curve.numKeys - 1;
            nax3Curve.preInfinityType = curve.preInfinityType;
            nax3Curve.postInfinityType = curve.postInfinityType;
            nax3Curve.curveType = curve.curveType;

            byteOrder.ConvertInPlace(nax3Curve.firstIntervalOffset);
            byteOrder.ConvertInPlace(nax3Curve.numIntervals);

            // write to stream
            stream->Write(&nax3Curve, sizeof(nax3Curve));            

            int stride = curve.curveType == CurveType::Rotation ? 4 : 3;

            // Create intervals for the keys
            for (IndexT i = 0; i < curve.numKeys - 1; i++)
            {
                Timing::Tick start = anim.keyTimes[curve.firstTimeOffset + i];
                Timing::Tick end = anim.keyTimes[curve.firstTimeOffset + i + 1];
                Nax3Interval interval;

                interval.start = byteOrder.Convert(start);
                interval.end = byteOrder.Convert(end);
                interval.key0 = byteOrder.Convert(curve.firstKeyOffset + i * stride);
                interval.key1 = byteOrder.Convert(curve.firstKeyOffset + (i + 1) * stride);

                intervals.Append(interval);
            }
        }

        for (const auto& animEvent : anim.events)
        {
            Nax3AnimEvent nax3AnimEvent;

            // check name restrictions
            const String& eventName = animEvent.name.AsString();
            const String& categoryName = animEvent.category.AsString();
            if (eventName.Length() >= sizeof(nax3AnimEvent.name))
            {
                n_error("AnimBuilderSaver: Anim event name too long! (file=%s, event=%s)\n",
                    stream->GetURI().LocalPath().AsCharPtr(),
                    eventName.AsCharPtr());
            }
            if (categoryName.Length() >= sizeof(nax3AnimEvent.category))
            {
                n_error("AnimBuilderServer: Anim event category too long! (file=%s, event=%s, category=%s)\n",
                    stream->GetURI().LocalPath().AsCharPtr(),
                    eventName.AsCharPtr(),
                    categoryName.AsCharPtr());
            }

            // write event attributes
            nax3AnimEvent.keyIndex = byteOrder.Convert(animEvent.time);
            eventName.CopyToBuffer(&(nax3AnimEvent.name[0]), sizeof(nax3AnimEvent.name));
            categoryName.CopyToBuffer(&(nax3AnimEvent.category[0]), sizeof(nax3AnimEvent.category));

            // write anim event to stream
            stream->Write(&nax3AnimEvent, sizeof(nax3AnimEvent));
        }

        uint curveOffset = 0, eventOffset = 0, velocityCurveOffset = 0;
        SizeT numClips = anim.GetNumClips();
        IndexT clipIndex;
        for (clipIndex = 0; clipIndex < numClips; clipIndex++)
        {
            AnimBuilderClip& clip = anim.GetClipAtIndex(clipIndex);
            Nax3Clip nax3Clip;

            // check clip name restrictions
            const String& clipName = clip.GetName().AsString();
            if (clipName.Length() >= sizeof(nax3Clip.name))
            {
                n_error("AnimBuilderSaver: Clip name '%s' is too long (%s)!\n", clipName.AsCharPtr(), stream->GetURI().LocalPath().AsCharPtr());
            }

            // write clip attributes
            clipName.CopyToBuffer(&(nax3Clip.name[0]), sizeof(nax3Clip.name));
            nax3Clip.firstCurve = byteOrder.Convert(curveOffset);
            nax3Clip.firstEvent = byteOrder.Convert(eventOffset);
            nax3Clip.firstVelocityCurve = byteOrder.Convert(velocityCurveOffset);
            nax3Clip.numCurves = byteOrder.Convert(clip.numCurves);
            nax3Clip.numEvents = byteOrder.Convert(clip.numEvents);
            nax3Clip.numVelocityCurves = byteOrder.Convert(clip.numVelocityCurves);
            nax3Clip.duration = byteOrder.Convert(clip.duration);

            curveOffset += clip.numCurves;
            eventOffset += clip.numEvents;
            velocityCurveOffset += clip.numVelocityCurves;

            // write clip header to stream
            stream->Write(&nax3Clip, sizeof(nax3Clip));
        }

        for (const auto& interval : intervals)
        {
            stream->Write(&interval, sizeof(Nax3Interval));
        }

        for (const float key : anim.keys)
        {
            float value = byteOrder.Convert(key);
            stream->Write(&value, sizeof(float));
        }
    }
}

} // namespace ToolkitUtil
