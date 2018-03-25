//------------------------------------------------------------------------------
//  animbuildersaver.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitutil/animutil/animbuildersaver.h"
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
AnimBuilderSaver::SaveNax3(const URI& uri, const AnimBuilder& animBuilder, Platform::Code platform)
{
    // make sure the target directory exists
    IoServer::Instance()->CreateDirectory(uri.LocalPath().ExtractDirName());

    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::WriteAccess);
    if (stream->Open())
    {
        ByteOrder byteOrder(ByteOrder::Host, Platform::GetPlatformByteOrder(platform));
        AnimBuilderSaver::WriteHeader(stream, animBuilder, byteOrder);
        AnimBuilderSaver::WriteClips(stream, animBuilder, byteOrder);
        AnimBuilderSaver::WriteKeys(stream, animBuilder, byteOrder);

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
AnimBuilderSaver::WriteHeader(const Ptr<Stream>& stream, const AnimBuilder& animBuilder, const ByteOrder& byteOrder)
{
    // setup header
    Nax3Header nax3Header;
    nax3Header.magic         = byteOrder.Convert<uint>(NEBULA3_NAX3_MAGICNUMBER);
    nax3Header.numClips      = byteOrder.Convert<uint>(animBuilder.GetNumClips());
    nax3Header.numKeys       = byteOrder.Convert<uint>(animBuilder.CountKeys());

    // write header
    stream->Write(&nax3Header, sizeof(nax3Header));
}

//------------------------------------------------------------------------------
/**
*/
void
AnimBuilderSaver::WriteClips(const Ptr<Stream>& stream, const AnimBuilder& animBuilder, const ByteOrder& byteOrder)
{
    SizeT numClips = animBuilder.GetNumClips();
    IndexT clipIndex;
    for (clipIndex = 0; clipIndex < numClips; clipIndex++)
    {
        AnimBuilderClip& clip = animBuilder.GetClipAtIndex(clipIndex);
        Nax3Clip nax3Clip;

        // check clip name restrictions
        const String& clipName = clip.GetName().AsString();
        if (clipName.Length() >= sizeof(nax3Clip.name))
        {
            n_error("AnimBuilderSaver: Clip name '%s' is too long (%s)!\n", clipName.AsCharPtr(), stream->GetURI().LocalPath().AsCharPtr());
        }
 
        // write clip attributes
        clipName.CopyToBuffer(&(nax3Clip.name[0]), sizeof(nax3Clip.name));
        nax3Clip.numCurves        = byteOrder.Convert<ushort>(clip.GetNumCurves());
        nax3Clip.startKeyIndex    = byteOrder.Convert<ushort>(clip.GetStartKeyIndex());
        nax3Clip.numKeys          = byteOrder.Convert<ushort>(clip.GetNumKeys());
        nax3Clip.keyStride        = byteOrder.Convert<ushort>(clip.GetKeyStride());
        nax3Clip.keyDuration      = byteOrder.Convert<ushort>(clip.GetKeyDuration());
        nax3Clip.preInfinityType  = clip.GetPreInfinityType();
        nax3Clip.postInfinityType = clip.GetPostInfinityType();
        nax3Clip.numEvents        = byteOrder.Convert<ushort>(clip.GetNumAnimEvents());

        // write clip header to stream
        stream->Write(&nax3Clip, sizeof(nax3Clip));

        // write clip anim events
        AnimBuilderSaver::WriteClipAnimEvents(stream, clip, byteOrder);

        // write clip curves
        AnimBuilderSaver::WriteClipCurves(stream, clip, byteOrder);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AnimBuilderSaver::WriteClipAnimEvents(const Ptr<Stream>& stream, AnimBuilderClip& clip, const ByteOrder& byteOrder)
{
    IndexT animEventIndex;
    for (animEventIndex = 0; animEventIndex < clip.GetNumAnimEvents(); animEventIndex++)
    {
        AnimEvent& animEvent = clip.GetAnimEventAtIndex(animEventIndex);
        Nax3AnimEvent nax3AnimEvent;

        // check name restrictions
        const String& eventName = animEvent.GetName().AsString();
        const String& categoryName = animEvent.GetCategory().AsString();
        if (eventName.Length() >= sizeof(nax3AnimEvent.name))
        {
            n_error("AnimBuilderSaver: Anim event name too long! (file=%s, clip=%s, event=%s)\n", 
                stream->GetURI().LocalPath().AsCharPtr(),
                clip.GetName().AsString().AsCharPtr(),
                eventName.AsCharPtr());
        }
        if (categoryName.Length() >= sizeof(nax3AnimEvent.category))
        {
            n_error("AnimBuilderServer: Anim event category too long! (file=%s, clip=%s, event=%s, category=%s)\n", 
                stream->GetURI().LocalPath().AsCharPtr(),
                clip.GetName().AsString().AsCharPtr(),
                eventName.AsCharPtr(),
                categoryName.AsCharPtr());
        }

        // write event attributes
        nax3AnimEvent.keyIndex  = byteOrder.Convert<ushort>(animEvent.GetTime());
        eventName.CopyToBuffer(&(nax3AnimEvent.name[0]), sizeof(nax3AnimEvent.name));
        categoryName.CopyToBuffer(&(nax3AnimEvent.category[0]), sizeof(nax3AnimEvent.category));

        // write anim event to stream
        stream->Write(&nax3AnimEvent, sizeof(nax3AnimEvent));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AnimBuilderSaver::WriteClipCurves(const Ptr<Stream>& stream, AnimBuilderClip& clip, const ByteOrder& byteOrder)
{
    SizeT numCurves = clip.GetNumCurves();
    IndexT curveIndex;
    for (curveIndex = 0; curveIndex < numCurves; curveIndex++)
    {
        AnimBuilderCurve& curve = clip.GetCurveAtIndex(curveIndex);

        // write curve attributes
        Nax3Curve nax3Curve;
        nax3Curve.firstKeyIndex = curve.GetFirstKeyIndex();
        nax3Curve.isActive = curve.IsActive();
        nax3Curve.isStatic = curve.IsStatic();
        nax3Curve.curveType = curve.GetCurveType();
        nax3Curve._padding = 0;
        nax3Curve.staticKeyX = curve.GetStaticKey().x();
        nax3Curve.staticKeyY = curve.GetStaticKey().y();
        nax3Curve.staticKeyZ = curve.GetStaticKey().z();
        nax3Curve.staticKeyW = curve.GetStaticKey().w();
        
        byteOrder.ConvertInPlace<uint>(nax3Curve.firstKeyIndex);
        byteOrder.ConvertInPlace<float>(nax3Curve.staticKeyX);
        byteOrder.ConvertInPlace<float>(nax3Curve.staticKeyY);
        byteOrder.ConvertInPlace<float>(nax3Curve.staticKeyZ);
        byteOrder.ConvertInPlace<float>(nax3Curve.staticKeyW);
        
        // write to stream
        stream->Write(&nax3Curve, sizeof(nax3Curve));
    }
}

//------------------------------------------------------------------------------
/**
*/
void
AnimBuilderSaver::WriteKeys(const Ptr<Stream>& stream, const AnimBuilder& animBuilder, const ByteOrder& byteOrder)
{
    SizeT numClips = animBuilder.GetNumClips();
    IndexT clipIndex;
    for (clipIndex = 0; clipIndex < numClips; clipIndex++)
    {
        AnimBuilderClip& clip = animBuilder.GetClipAtIndex(clipIndex);
        SizeT numCurves = clip.GetNumCurves();
        SizeT numKeys = clip.GetNumKeys();
        IndexT keyIndex;
        for (keyIndex = 0; keyIndex < numKeys; keyIndex++)
        {
            IndexT curveIndex;
            for (curveIndex = 0; curveIndex < numCurves; curveIndex++)
            {
                const AnimBuilderCurve& curve = clip.GetCurveAtIndex(curveIndex);
                if (!curve.IsStatic())
                {
                    const float4& key = clip.GetCurveAtIndex(curveIndex).GetKey(keyIndex);
                    float keyValues[4];
                    keyValues[0] = key.x();
                    keyValues[1] = key.y();
                    keyValues[2] = key.z();
                    keyValues[3] = key.w();
                    byteOrder.ConvertInPlace<float>(keyValues[0]);
                    byteOrder.ConvertInPlace<float>(keyValues[1]);
                    byteOrder.ConvertInPlace<float>(keyValues[2]);
                    byteOrder.ConvertInPlace<float>(keyValues[3]);
                    stream->Write(keyValues, sizeof(keyValues));
                }
            }
        }
    }
}

} // namespace ToolkitUtil
