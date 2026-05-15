//------------------------------------------------------------------------------
//  animbuildersaver.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "model/animutil/animbuildersaver.h"
#include "io/ioserver.h"
#include "coreanimation/naxfileformatstructs.h"

#include "nflatbuffer/flatbufferinterface.h"
#include "nflatbuffer/nebula_flat.h"
#include "flat/anim.h"

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
AnimBuilderSaver::SaveImport(const URI& uri, const Util::Array<AnimBuilder>& animBuilders, Platform::Code platform)
{
    // make sure the target directory exists
    IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        ToolkitUtil::AnimResourceT anims;
        for (const auto& builder : animBuilders)
        {
            auto anim = std::make_unique<ToolkitUtil::AnimInstanceT>();
            anim->keys = { builder.keys.Begin(), builder.keys.End() };
            anim->key_times = {builder.keyTimes.Begin(), builder.keyTimes.End()};

            for (const auto& curve : builder.curves)
            {
                ToolkitUtil::AnimCurve curveT(curve.firstKeyOffset, curve.firstTimeOffset, curve.numKeys, curve.curveType, curve.preInfinityType, curve.postInfinityType);
                anim->curves.push_back(std::move(curveT));
            }
            for (const auto& event : builder.events)
            {
                auto eventT = std::make_unique<ToolkitUtil::AnimEventT>();
                eventT->name = event.name.AsString();
                eventT->category = event.category.AsString();
                eventT->time = event.time;
                anim->events.push_back(std::move(eventT));
            }
            for (const auto& clip : builder.clips)
            {
                auto clipT = std::make_unique<ToolkitUtil::AnimClipT>();
                clipT->name = clip.GetName().AsString();
                clipT->first_curve_offset = clip.firstCurveOffset;
                clipT->num_curves = clip.numCurves;
                clipT->first_event_offset = clip.firstEventOffset;
                clipT->num_events = clip.numEvents;
                clipT->first_velocity_curve_offset = clip.firstVelocityCurveOffset;
                clipT->num_velocity_curves = clip.numVelocityCurves;
                clipT->duration = clip.duration;
                anim->clips.push_back(std::move(clipT));
            }
            anims.animations.push_back(std::move(anim));
        }

        Util::Blob data = Flat::FlatbufferInterface::SerializeFlatbuffer<ToolkitUtil::AnimResource>(anims);
        stream->Write(data.GetPtr(), data.Size());

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
bool
AnimBuilderSaver::SaveBinary(const URI& uri, const ToolkitUtil::AnimResourceT* resource, Platform::Code platform)
{
    // make sure the target directory exists
    IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        ByteOrder byteOrder(ByteOrder::Host, Platform::GetPlatformByteOrder(platform));
        AnimBuilderSaver::WriteHeader(stream, resource, byteOrder);
        AnimBuilderSaver::WriteAnimations(stream, resource, byteOrder);

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
AnimBuilderSaver::WriteHeader(const Ptr<Stream>& stream, const ToolkitUtil::AnimResourceT* resource, const ByteOrder& byteOrder)
{
    // setup header
    Nax3Header nax3Header;
    nax3Header.magic         = byteOrder.Convert<uint>(NEBULA_NAX3_MAGICNUMBER);
    nax3Header.numAnimations = byteOrder.Convert(resource->animations.size());

    // write header
    stream->Write(&nax3Header, sizeof(nax3Header));
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimBuilderSaver::WriteAnimations(const Ptr<IO::Stream>& stream, const ToolkitUtil::AnimResourceT* resource, const System::ByteOrder& byteOrder)
{
    for (auto& anim : resource->animations)
    {
        Nax3Anim nax3;
        nax3.numClips = anim->clips.size();
        nax3.numEvents = anim->events.size();
        nax3.numCurves = anim->curves.size();
        nax3.numKeys = anim->keys.size();
        nax3.numIntervals = 0;
        for (const auto& curve : anim->curves)
        {
            nax3.numIntervals += curve.num_keys() == 0 ? 0 : curve.num_keys() - 1;
        }

        // write header
        stream->Write(&nax3, sizeof(nax3));

        Util::Array<Nax3Interval> intervals;

        for (const auto& curve : anim->curves)
        {
            // write curve attributes
            Nax3Curve nax3Curve;
            nax3Curve.firstIntervalOffset = intervals.Size();
            nax3Curve.numIntervals = curve.num_keys() == 0 ? 0 : curve.num_keys() - 1;
            nax3Curve.preInfinityType = curve.pre_infinity();
            nax3Curve.postInfinityType = curve.post_infinity();
            nax3Curve.curveType = curve.type();

            byteOrder.ConvertInPlace(nax3Curve.firstIntervalOffset);
            byteOrder.ConvertInPlace(nax3Curve.numIntervals);

            // write to stream
            stream->Write(&nax3Curve, sizeof(nax3Curve));            

            int stride = curve.type() == CurveType::Rotation ? 4 : 3;

            // Create intervals for the keys
            for (IndexT i = 0; i < nax3Curve.numIntervals; i++)
            {
                Timing::Tick start = anim->key_times[curve.first_time_offset() + i];
                Timing::Tick end = anim->key_times[curve.first_time_offset() + i + 1];
                Nax3Interval interval;

                interval.start = byteOrder.Convert(start);
                interval.end = byteOrder.Convert(end);
                interval.key0 = byteOrder.Convert(curve.first_key_offset() + i * stride);
                interval.key1 = byteOrder.Convert(curve.first_key_offset() + (i + 1) * stride);
                interval.duration = 1 / float(interval.end - interval.start);

                intervals.Append(interval);
            }
        }

        for (const auto& animEvent : anim->events)
        {
            Nax3AnimEvent nax3AnimEvent;

            // check name restrictions
            const String& eventName = animEvent->name;
            const String& categoryName = animEvent->category;
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
            nax3AnimEvent.keyIndex = byteOrder.Convert(animEvent->time);
            eventName.CopyToBuffer(&(nax3AnimEvent.name[0]), sizeof(nax3AnimEvent.name));
            categoryName.CopyToBuffer(&(nax3AnimEvent.category[0]), sizeof(nax3AnimEvent.category));

            // write anim event to stream
            stream->Write(&nax3AnimEvent, sizeof(nax3AnimEvent));
        }

        uint curveOffset = 0, eventOffset = 0, velocityCurveOffset = 0;
        SizeT numClips = anim->clips.size();
        IndexT clipIndex;
        for (clipIndex = 0; clipIndex < numClips; clipIndex++)
        {
            const auto& clip = anim->clips[clipIndex];
            Nax3Clip nax3Clip;

            // check clip name restrictions
            const String& clipName = clip->name;
            if (clipName.Length() >= sizeof(nax3Clip.name))
            {
                n_error("AnimBuilderSaver: Clip name '%s' is too long (%s)!\n", clipName.AsCharPtr(), stream->GetURI().LocalPath().AsCharPtr());
            }

            // write clip attributes
            clipName.CopyToBuffer(&(nax3Clip.name[0]), sizeof(nax3Clip.name));
            nax3Clip.firstCurve = byteOrder.Convert(curveOffset);
            nax3Clip.firstEvent = byteOrder.Convert(eventOffset);
            nax3Clip.firstVelocityCurve = byteOrder.Convert(velocityCurveOffset);
            nax3Clip.numCurves = byteOrder.Convert(clip->num_curves);
            nax3Clip.numEvents = byteOrder.Convert(clip->num_events);
            nax3Clip.numVelocityCurves = byteOrder.Convert(clip->num_velocity_curves);
            nax3Clip.duration = byteOrder.Convert(clip->duration);

            curveOffset += clip->num_curves;
            eventOffset += clip->num_events;
            velocityCurveOffset += clip->num_velocity_curves;

            // write clip header to stream
            stream->Write(&nax3Clip, sizeof(nax3Clip));
        }

        for (const auto& interval : intervals)
        {
            stream->Write(&interval, sizeof(Nax3Interval));
        }

        for (const float key : anim->keys)
        {
            float value = byteOrder.Convert(key);
            stream->Write(&value, sizeof(float));
        }
    }
}

} // namespace ToolkitUtil
