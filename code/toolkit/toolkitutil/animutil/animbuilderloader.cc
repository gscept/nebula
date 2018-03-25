//------------------------------------------------------------------------------
//  animbuilderloader.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitutil/animutil/animbuilderloader.h"
#include "io/stream.h"
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
AnimBuilderLoader::LoadNax2(const URI& uri, AnimBuilder& animBuilder, const Array<String>& extClipNames, bool autoGenerateClipNames)
{
    Ptr<Stream> stream = IoServer::Instance()->CreateStream(uri);
    stream->SetAccessMode(Stream::ReadAccess);
    if (stream->Open())
    {
        uchar* ptr = (uchar*) stream->Map();

        // read header
        Nax2Header* naxHeader = (Nax2Header*) ptr;
        ptr += sizeof(Nax2Header);

        // check magic value
        if (FourCC(naxHeader->magic) != 'NAX3')
        {
            n_error("nMemoryAnimation::SetupFromNax2(): '%s' has obsolete file format!", stream->GetURI().AsString().AsCharPtr());        
            return false;
        }
        n_assert(0 != naxHeader->numGroups);

        // generate clip names if requested
        Array<String> clipNames;
        clipNames.Reserve(naxHeader->numGroups);
        if (autoGenerateClipNames)
        {
            String clipName;
            IndexT i;
            for (i = 0; i < naxHeader->numGroups; i++)
            {
                clipName.Format("clip%d", i);
                clipNames.Append(clipName);
            }
        }
        else if (naxHeader->numGroups != extClipNames.Size())
        {
            n_error("Number of clips in NAX2 file doesn't match number of provided clip names in '%s'.",
                uri.LocalPath().AsCharPtr());
        }
        else
        {
            clipNames = extClipNames;
        }

        // setup animation clips
        animBuilder.Reserve(naxHeader->numGroups);
        IndexT clipIndex;
        for (clipIndex = 0; clipIndex < naxHeader->numGroups; clipIndex++)
        {
            Nax2Group* naxGroup = (Nax2Group*) ptr;

            // setup AnimBuilderClip object
            AnimBuilderClip clip;
            clip.ReserveCurves(naxGroup->numCurves);
            clip.SetName(clipNames[clipIndex]);
            clip.SetNumKeys(naxGroup->numKeys);
            clip.SetKeyStride(naxGroup->keyStride);
            clip.SetKeyDuration(Timing::SecondsToTicks(naxGroup->keyTime));
            clip.SetStartKeyIndex(naxGroup->startKey);
            if (0 == naxGroup->loopType)
            {
                // Clamp
                clip.SetPreInfinityType(InfinityType::Constant);
                clip.SetPostInfinityType(InfinityType::Constant);
            }
            else
            {
                clip.SetPreInfinityType(InfinityType::Cycle);
                clip.SetPostInfinityType(InfinityType::Cycle);
            }
            String metaData = naxGroup->metaData;
            AnimBuilderLoader::ExtractAnimEventsFromNax2MetaData(clip, metaData);

            // add dummy curve objects to clip
            IndexT curveIndex;
            for (curveIndex = 0; curveIndex < naxGroup->numCurves; curveIndex++)
            {
                AnimBuilderCurve curve;
                clip.AddCurve(curve);
            }
            animBuilder.AddClip(clip);

            ptr += sizeof(Nax2Group);
        }

        // setup animation curves
        for (clipIndex = 0; clipIndex < naxHeader->numGroups; clipIndex++)
        {
            AnimBuilderClip& clip = animBuilder.GetClipAtIndex(clipIndex);
            IndexT curveIndex;
            for (curveIndex = 0; curveIndex < clip.GetNumCurves(); curveIndex++)
            {
                AnimBuilderCurve& curve = clip.GetCurveAtIndex(curveIndex);

                Nax2Curve* naxCurve = (Nax2Curve*) ptr;
                
                bool isStaticCurve = (-1 == naxCurve->firstKeyIndex);
                if (isStaticCurve)
                {
                    curve.SetFirstKeyIndex(0);
                }
                else
                {
                    curve.SetFirstKeyIndex(naxCurve->firstKeyIndex);
                }
                curve.SetStatic(isStaticCurve);
                curve.SetStaticKey(float4(naxCurve->keyX, naxCurve->keyY, naxCurve->keyZ, naxCurve->keyW));
                curve.SetActive(0 != naxCurve->isAnim);

                // this is a hack, Nebula2 files usually always have translation,
                // rotation and scale (in that order)
                int type = curveIndex % 3;
                switch (type)
                {
                    case 0:
                        curve.SetCurveType(CurveType::Translation);
                        break;
                    case 1:
                        n_assert(2 == naxCurve->ipolType);   // nAnimation::IpolType::Quat
                        curve.SetCurveType(CurveType::Rotation);
                        break;
                    case 2:
                        curve.SetCurveType(CurveType::Scale);
                        break;
                }

                // setup keys array
                if (!curve.IsStatic())
                {
                    curve.ResizeKeyArray(clip.GetNumKeys());
                }

                // advance to next NAX2 curve
                ptr += sizeof(Nax2Curve);
            }
        }

        // finally, setup keys
        float4 curKey;
        const float4* keyBasePtr = (const float4*) ptr;
        for (clipIndex = 0; clipIndex < naxHeader->numGroups; clipIndex++)
        {
            AnimBuilderClip& clip = animBuilder.GetClipAtIndex(clipIndex);
            IndexT curveIndex;
            for (curveIndex = 0; curveIndex < clip.GetNumCurves(); curveIndex++)
            {
                AnimBuilderCurve& curve = clip.GetCurveAtIndex(curveIndex);
                if (!curve.IsStatic())
                {
                    IndexT keyIndex;
                    for (keyIndex = 0; keyIndex < clip.GetNumKeys(); keyIndex++)
                    {
                        const float4* keyPtr = keyBasePtr + curve.GetFirstKeyIndex() + keyIndex * clip.GetKeyStride();

                        // need to load un-aligned!
                        curKey.loadu((scalar*)keyPtr);
                        curve.SetKey(keyIndex, curKey);
                    }
                }
            }
        }
        stream->Unmap();
        stream->Close();
        stream = nullptr;

        animBuilder.FixInactiveCurveStaticKeyValues();
        return true;
    }
    else
    {
        // failed to open file
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    AAAARRRRRRGHHHHHHHH: This whole MetaData crap in NAX2 is bullshit!
*/
void
AnimBuilderLoader::ExtractAnimEventsFromNax2MetaData(AnimBuilderClip& clip, const String& metaDataString)
{
    String searchString("Hotspot");
    Array<String> metaDataParts = metaDataString.Tokenize(";");
    IndexT idxMetaDataParts;
    for (idxMetaDataParts = 0; idxMetaDataParts < metaDataParts.Size(); idxMetaDataParts++)
    {
        IndexT startIndex = metaDataParts[idxMetaDataParts].FindStringIndex("Hotspot");
        if (InvalidIndex != startIndex)
        {
            // setup hotspots
            String hotspotsString = metaDataParts[idxMetaDataParts].ExtractToEnd(startIndex + searchString.Length() + 1);
            Array<String> hotspots = hotspotsString.Tokenize(">");

            // skip if this hotspot section is empty
            if (hotspots.Size() == 0)
            {
                continue;
            }

            IndexT idxHotspot;
            String name;
            Array<String> times;

            for (idxHotspot = 0; idxHotspot < hotspots.Size(); idxHotspot++)
            {
                Array<String> hotspot = hotspots[idxHotspot].Tokenize("#");
                
                // fallback
                if (1 == hotspot.Size())
                {
                    name = "default";
                    times = hotspot[0].Tokenize(",");
                }
                else if (2 == hotspot.Size())
                {
                    name = hotspot[0];
                    times = hotspot[1].Tokenize(",");
                }
                else
                {
                    n_error("AnimBuilderLoader: Hotspot has wrong format!\n");
                }
                
                if (0 == times.Size())
                {
                    n_error("AnimBuilderLoader: Hotspot '%s' has no time!\n", name.AsCharPtr());
                }

                IndexT idxTime;
                for (idxTime = 0; idxTime < times.Size(); idxTime++)
                {
                    // check time
                    if (!times[idxTime].IsValidFloat())
                    {
                        n_error("AnimBuilderLoader: Invalid Hotspot time '%s' on Hotspot '%s' (valid float expected)!\n",
                            times[idxTime].AsCharPtr(), name.AsCharPtr());
                    }

                    // split string into category and name
                    Util::StringAtom category;
                    Util::Array<Util::String> strTokens = name.Tokenize(":");
                    if (strTokens.Size() == 2)
                    {
                        category = strTokens[0];
                        name = strTokens[1];
                    }

                    // append hotspot
                    // NOTE: the time is written as frame number!
                    AnimEvent newEvent;
                    newEvent.SetName(name);
                    newEvent.SetCategory(category);
                    newEvent.SetTime((Timing::Tick)times[idxTime].AsFloat());
                    clip.AddEvent(newEvent);
                }
            }
        }
    }
}

} // namespace ToolkitUtil