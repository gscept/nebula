//------------------------------------------------------------------------------
//  animbuilder.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitutil/animutil/animbuilder.h"

namespace ToolkitUtil
{
using namespace Util;
using namespace CoreAnimation;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
AnimBuilder::AnimBuilder()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
AnimBuilder::Clear()
{
    this->clipArray.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
AnimBuilder::Reserve(SizeT numClips)
{
    this->clipArray.Reserve(numClips);
}

//------------------------------------------------------------------------------
/**
*/
void
AnimBuilder::AddClip(const AnimBuilderClip& clip)
{
    this->clipArray.Append(clip);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
AnimBuilder::CountCurves() const
{
    SizeT numCurves = 0;
    IndexT clipIndex;
    for (clipIndex = 0; clipIndex < this->clipArray.Size(); clipIndex++)
    {
        numCurves += this->clipArray[clipIndex].GetNumCurves();
    }
    return numCurves;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
AnimBuilder::CountEvents() const
{
    SizeT numEvents = 0;
    IndexT clipIndex;
    for (clipIndex = 0; clipIndex < this->clipArray.Size(); clipIndex++)
    {
        numEvents += this->clipArray[clipIndex].GetNumAnimEvents();
    }
    return numEvents;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
AnimBuilder::CountKeys() const
{
    SizeT numKeys = 0;
    IndexT clipIndex;
    for (clipIndex = 0; clipIndex < this->clipArray.Size(); clipIndex++)
    {
        AnimBuilderClip& clip = this->clipArray[clipIndex];
        IndexT curveIndex;
        for (curveIndex = 0; curveIndex < clip.GetNumCurves(); curveIndex++)
        {
            AnimBuilderCurve& curve = clip.GetCurveAtIndex(curveIndex);
            if (!curve.IsStatic())
            {
                numKeys += curve.GetNumKeys();
            }
        }
    }
    return numKeys;
}

//------------------------------------------------------------------------------
/**
    For each translation curve, add a velocity curve. The velocity curve
    will be inserted after the scale curve and the next translation curve.

    FIXME: To compute a really meaningful velocity, we would have
    to convert the joint positions to model space first!! At the moment
    this is only really useful for AnimDrivenMotion on the root index!
*/
void
AnimBuilder::BuildVelocityCurves()
{    
    IndexT clipIndex;
    for (clipIndex = 0; clipIndex < this->clipArray.Size(); clipIndex++)
    {
        // first build an array of velocity curves
        Array<AnimBuilderCurve> velocityCurves;
        AnimBuilderClip& clip = this->clipArray[clipIndex];
        IndexT curveIndex;
        for (curveIndex = 0; curveIndex < clip.GetNumCurves(); curveIndex++)
        {
            AnimBuilderCurve& srcCurve = clip.GetCurveAtIndex(curveIndex);
            if (srcCurve.GetCurveType() == CurveType::Translation)
            {
                AnimBuilderCurve dstCurve;
                dstCurve.SetActive(srcCurve.IsActive());
                dstCurve.SetCurveType(CurveType::Velocity);
                if (srcCurve.IsStatic())
                {
                    // if translation is static, velocity is 0
                    dstCurve.SetStatic(true);
                    dstCurve.SetStaticKey(float4(0.0f, 0.0f, 0.0f, 0.0f));
                }
                else
                {
                    float scale = float(1.0 / Timing::TicksToSeconds(clip.GetKeyDuration()));
                    dstCurve.ResizeKeyArray(clip.GetNumKeys());

                    // compute velocity keys and add to curve
                    // (velocity is in meter/sec)
                    float4 velocity(0.0f, 0.0f, 0.0f, 0.0f);
                    IndexT keyIndex;
                    for (keyIndex = 0; keyIndex < srcCurve.GetNumKeys() - 1; keyIndex++)
                    {
                        float4 key0 = srcCurve.GetKey(keyIndex);
                        float4 key1 = srcCurve.GetKey(keyIndex + 1);
                        velocity = (key1 - key0) * scale;
                        dstCurve.SetKey(keyIndex, velocity);
                    }

                    // clips for anim-driven-motion have one extra key at the
                    // end which is identical with the first key, this we can just duplicate
                    // the last valid key
                    dstCurve.SetKey(keyIndex, velocity);
                }
                velocityCurves.Append(dstCurve);
            }
        }

        // then insert the velocity curves into the clip's curve array
        n_assert(velocityCurves.Size() == (clip.GetNumCurves() / 3));
        for (curveIndex = 0; curveIndex < velocityCurves.Size(); curveIndex++)
        {
            clip.InsertCurve((curveIndex * 3) + 3 + curveIndex, velocityCurves[curveIndex]);            
        }
    }

    // when everything is done we need to fix the first-key-indices in the animation curves,
    // also make sure that good static key values are set for inactive curves
    this->FixAnimCurveFirstKeyIndices();
    this->FixInactiveCurveStaticKeyValues();
}

//------------------------------------------------------------------------------
/**
    This re-computes the FirstKeyIndices in the AnimBuilderCurves.
*/
void
AnimBuilder::FixAnimCurveFirstKeyIndices()
{
    IndexT clipFirstKeyIndex = 0;
    IndexT clipIndex;
    for (clipIndex = 0; clipIndex < this->clipArray.Size(); clipIndex++)
    {
        AnimBuilderClip& clip = this->clipArray[clipIndex];
        SizeT numNonStaticCurvesInClip = 0;
        IndexT curveIndex;
        for (curveIndex = 0; curveIndex < clip.GetNumCurves(); curveIndex++)
        {
            AnimBuilderCurve& curve = clip.GetCurveAtIndex(curveIndex);
            if (!curve.IsStatic())
            {
                curve.SetFirstKeyIndex(clipFirstKeyIndex + numNonStaticCurvesInClip);
                numNonStaticCurvesInClip++;
            }
        }

        // the key stride in each clip is identical with the number of non-static curves
        clip.SetKeyStride(numNonStaticCurvesInClip);

        // update the first key index for next clip
        clipFirstKeyIndex += (numNonStaticCurvesInClip * clip.GetNumKeys());
    }
}

//------------------------------------------------------------------------------
/**
    Fixes the static keys in inactive animation curves, those must have
    suitable values for the curve type (e.g. (1,1,1,0) for a scaling curve).
*/
void
AnimBuilder::FixInactiveCurveStaticKeyValues()
{
    IndexT clipIndex;
    for (clipIndex = 0; clipIndex < this->clipArray.Size(); clipIndex++)
    {
        AnimBuilderClip& clip = this->clipArray[clipIndex];
        IndexT curveIndex;
        for (curveIndex = 0; curveIndex < clip.GetNumCurves(); curveIndex++)
        {
            AnimBuilderCurve& curve = clip.GetCurveAtIndex(curveIndex);
            if (!curve.IsActive())
            {
                switch (curve.GetCurveType())
                {
                    case CurveType::Translation:    curve.SetStaticKey(float4(0.0f, 0.0f, 0.0f, 0.0f)); break;
                    case CurveType::Rotation:       curve.SetStaticKey(float4(0.0f, 0.0f, 0.0f, 1.0f)); break;
                    case CurveType::Scale:          curve.SetStaticKey(float4(1.0f, 1.0f, 1.0f, 0.0f)); break;
                    case CurveType::Velocity:       curve.SetStaticKey(float4(0.0f, 0.0f, 0.0f, 0.0f)); break;
                    default:                        curve.SetStaticKey(float4(0.0f, 0.0f, 0.0f, 0.0f)); break;
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
    Trim a number of keys from the end of each animation curve.
*/
void
AnimBuilder::TrimEnd(SizeT numTrimKeys)
{
    IndexT clipIndex;
    for (clipIndex = 0; clipIndex < this->clipArray.Size(); clipIndex++)
    {
        AnimBuilderClip& clip = this->clipArray[clipIndex];
        
        // trim number of keys in clip
        SizeT newNumKeys = clip.GetNumKeys() - numTrimKeys;
        n_assert(newNumKeys > 0);
        clip.SetNumKeys(newNumKeys);

        // also need to trim curves
        IndexT curveIndex;
        for (curveIndex = 0; curveIndex < clip.GetNumCurves(); curveIndex++)
        {
            AnimBuilderCurve& curve = clip.GetCurveAtIndex(curveIndex);
            if (!curve.IsStatic())
            {
                curve.ResizeKeyArray(newNumKeys);
            }
        }
    }

    // need to re-compute first-key-indices after trimming!
    this->FixAnimCurveFirstKeyIndices();
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimBuilder::FixInvalidKeyValues()
{
	IndexT clipIndex;
	for (clipIndex = 0; clipIndex < this->clipArray.Size(); clipIndex++)
	{
		AnimBuilderClip& clip = this->clipArray[clipIndex];
		IndexT curveIndex;
		for (curveIndex = 0; curveIndex < clip.GetNumCurves(); curveIndex++)
		{
			AnimBuilderCurve& curve = clip.GetCurveAtIndex(curveIndex);
			curve.FixInvalidKeys();
		}
	}
}

} // namespace ToolkitUtil