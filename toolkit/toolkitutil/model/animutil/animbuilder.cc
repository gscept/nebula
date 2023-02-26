//------------------------------------------------------------------------------
//  animbuilder.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "model/animutil/animbuilder.h"

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
    this->clips.Clear();
}

//------------------------------------------------------------------------------
/**
*/
void
AnimBuilder::Reserve(SizeT numClips)
{
    this->clips.Reserve(numClips);
}

//------------------------------------------------------------------------------
/**
*/
void
AnimBuilder::AddClip(const AnimBuilderClip& clip)
{
    this->clips.Append(clip);
}

//------------------------------------------------------------------------------
/**
*/
SizeT
AnimBuilder::CountCurves() const
{
    return this->curves.Size();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
AnimBuilder::CountEvents() const
{
    return this->events.Size();
}

//------------------------------------------------------------------------------
/**
*/
SizeT
AnimBuilder::CountKeys() const
{
    return this->keys.Size();
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
AnimBuilder::BuildVelocityCurves(float keysPerMS)
{    
    IndexT velocityCurveOffset = 0;
    for (auto& clip : this->clips)
    {
        // first build an array of velocity curves
        Array<AnimBuilderCurve> velocityCurves;
        IndexT curveIndex;
        for (curveIndex = 0; curveIndex < clip.numCurves; curveIndex++)
        {
            AnimBuilderCurve& srcCurve = this->curves[curveIndex + clip.firstCurveOffset];
            if (srcCurve.curveType == CurveType::Translation)
            {
                AnimBuilderCurve dstCurve;
                dstCurve.curveType = CurveType::Velocity;
                dstCurve.preInfinityType = srcCurve.preInfinityType;
                dstCurve.postInfinityType = srcCurve.postInfinityType;
                this->keys.Reserve(srcCurve.numKeys * 3);
                this->keyTimes.Reserve(srcCurve.numKeys);
                float scale = float(1.0 / Timing::TicksToSeconds(keysPerMS));

                auto velocityFunc = [&](Util::Array<float>& keys, Util::Array<Timing::Tick>& keyTimes, const AnimBuilderCurve& src, AnimBuilderCurve& dst)
                {
                    if (src.numKeys == 0)
                    {
                        dst.firstKeyOffset = 0;
                        dst.firstTimeOffset = 0;
                        dst.numKeys = 0;
                        return;
                    }

                    dst.firstKeyOffset = keys.Size();
                    dst.firstTimeOffset = keyTimes.Size();
                    dst.numKeys = src.numKeys - 1;
                    for (IndexT keyIndex = 0, timeIndex = 0; keyIndex < dst.numKeys; keyIndex++)
                    {
                        float key0X = this->keys[src.firstKeyOffset + keyIndex * 3];
                        float key0Y = this->keys[src.firstKeyOffset + keyIndex * 3 + 1];
                        float key0Z = this->keys[src.firstKeyOffset + keyIndex * 3 + 2];
                        float key1X = this->keys[src.firstKeyOffset + keyIndex * 3 + 3];
                        float key1Y = this->keys[src.firstKeyOffset + keyIndex * 3 + 4];
                        float key1Z = this->keys[src.firstKeyOffset + keyIndex * 3 + 5];

                        float velocity[] =
                        {
                            (key1X - key0X) * scale,
                            (key1Y - key0Y) * scale,
                            (key1Z - key0Z) * scale,
                        };
                        keys.Append(velocity[0]);
                        keys.Append(velocity[1]);
                        keys.Append(velocity[2]);
                        keyTimes.Append(this->keyTimes[src.firstTimeOffset + keyIndex]);
                    }
                };
                velocityFunc(this->keys, this->keyTimes, srcCurve, dstCurve);
                velocityCurves.Append(dstCurve);
            }
        }

        // then insert the velocity curves into the clip's curve array
        n_assert(velocityCurves.Size() == (clip.numCurves / 3));
        this->curves.AppendArray(velocityCurves);
        clip.firstVelocityCurveOffset = velocityCurveOffset;
        clip.numVelocityCurves = velocityCurves.Size();

        velocityCurveOffset += velocityCurves.Size();
    }    
}

} // namespace ToolkitUtil