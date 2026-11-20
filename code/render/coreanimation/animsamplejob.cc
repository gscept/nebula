//------------------------------------------------------------------------------
//  animsamplejob.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "animsamplemask.h"
#include "animkeybuffer.h"
#include "animcurve.h"
#include "animclip.h"

using namespace Math;
namespace CoreAnimation
{

//------------------------------------------------------------------------------
/**
*/
inline Timing::Tick
WrapTime(
    const AnimCurve& curve
    , Timing::Tick time
    , Timing::Tick lastTime
)
{
    if (time >= lastTime)
    {
        return lastTime;
    }
    else
        return time;
}

//------------------------------------------------------------------------------
/**
*/
AnimKeyBuffer::Interval
FindNextInterval(
    const AnimCurve& curve
    , const Timing::Tick time
    , uint& key
    , const AnimKeyBuffer::Interval* sampleTimes
)
{
    if (time < sampleTimes[curve.firstIntervalOffset + key].start)
        key = 0;

    AnimKeyBuffer::Interval interval = sampleTimes[curve.firstIntervalOffset + key];

    // Check each time if it's behind the current time.
    // We are looking the find the first key after the current sample time, such that
    // key - 1 and key is before and after the current sample time. 
    while (key < curve.numIntervals - 1)
    {
        if (time >= interval.start && time < interval.end)
            break;
        else
            ++key;

        interval = sampleTimes[curve.firstIntervalOffset + key];
    }

    return interval;
}

//------------------------------------------------------------------------------
/**
*/
void
AnimSampleStep(
    const AnimClip& clip,
    const Util::FixedArray<AnimCurve>& curves,
    const Timing::Tick time,
    const vec4& velocityScale,
    const Util::FixedArray<Math::vec4>& idleSamples,
    const float* srcSamplePtr,
    const AnimKeyBuffer::Interval* intervalPtr,
    uint* lastUsedIntervalPtr,
    float* outSamplePtr,
    uchar* outSampleCounts)
{
    int i;
    for (i = 0; i < clip.numCurves; i ++)
    {
        const AnimCurve& curve = curves[clip.firstCurve + i];
        const bool activeCurve = curve.numIntervals > 0;
        int stride = 0;

        uint key = *lastUsedIntervalPtr;
        AnimKeyBuffer::Interval currentTime = intervalPtr[key];
        if (activeCurve)
        {
            AnimKeyBuffer::Interval curveLastTime = intervalPtr[curve.firstIntervalOffset + curve.numIntervals - 1];
            Timing::Tick wrappedTime = WrapTime(curve, time, curveLastTime.end);
            currentTime = FindNextInterval(curve, wrappedTime, key, intervalPtr);
        }

        switch (curve.curveType)
        {
            case CurveType::Rotation:
            {
                if (activeCurve)
                    memcpy(outSamplePtr, &srcSamplePtr[currentTime.key0], 4 * sizeof(float));
                else
                    idleSamples[i].storeu(outSamplePtr);
                stride = 4;
                break;
            }
            case CurveType::Scale:
            case CurveType::Velocity:
            case CurveType::Translation:
            {
                if (activeCurve)
                    memcpy(outSamplePtr, &srcSamplePtr[currentTime.key0], 3 * sizeof(float));
                else
                    idleSamples[i].storeu(outSamplePtr);
                stride = 3;
                break;
            }
            default: n_error("unhandled enum"); break;
        }

        if (curve.curveType == CurveType::Velocity)
        {
            outSamplePtr[0] = outSamplePtr[0] * velocityScale.x;
            outSamplePtr[1] = outSamplePtr[1] * velocityScale.y;
            outSamplePtr[2] = outSamplePtr[2] * velocityScale.z;
        }

        outSamplePtr += stride;

        *outSampleCounts = activeCurve;
        ++outSampleCounts;

        *lastUsedIntervalPtr = key;
        ++lastUsedIntervalPtr;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimSampleLinear(
    const AnimClip& clip,
    const Util::FixedArray<AnimCurve>& curves,
    const Timing::Tick time,
    const vec4& velocityScale,
    const Util::FixedArray<Math::vec4>& idleSamples,
    const float* srcSamplePtr,
    const AnimKeyBuffer::Interval* intervalPtr,
    uint* lastUsedIntervalPtr,
    float* outSamplePtr,
    uchar* outSampleCounts)
{
    int i;
    for (i = 0; i < clip.numCurves; i++)
    {
        const AnimCurve& curve = curves[clip.firstCurve + i];
        const bool activeCurve = curve.numIntervals > 0;

        float sampleWeight = 0.0f;
        int stride = 0;

        uint key = *lastUsedIntervalPtr;
        AnimKeyBuffer::Interval currentTime = intervalPtr[key];
        if (activeCurve)
        {
            AnimKeyBuffer::Interval curveLastTime = intervalPtr[curve.firstIntervalOffset + curve.numIntervals - 1];
            Timing::Tick wrappedTime = WrapTime(curve, time, curveLastTime.end);
            currentTime = FindNextInterval(curve, wrappedTime, key, intervalPtr);

            if (wrappedTime >= currentTime.end)
                sampleWeight = 1.0f;
            else
                sampleWeight = (wrappedTime - currentTime.start) * currentTime.duration;
        }

        switch (curve.curveType)
        {
            case CurveType::Rotation:
            {
                Math::quat q0;
                if (activeCurve)
                {
                    Math::quat q1;
                    q0.loadu(&srcSamplePtr[currentTime.key0]);
                    q1.loadu(&srcSamplePtr[currentTime.key1]);
                    q0 = Math::slerp(q0, q1, sampleWeight);
                }
                else
                    q0 = idleSamples[i].vec;

                q0.storeu(outSamplePtr);
                stride = 4;
                break;
            }
            case CurveType::Scale:
            case CurveType::Velocity:
            case CurveType::Translation:
            {
                Math::vec3 v0;
                if (activeCurve)
                {
                    Math::vec3 v1;
                    v0.loadu(&srcSamplePtr[currentTime.key0]);
                    v1.loadu(&srcSamplePtr[currentTime.key1]);
                    v0 = Math::lerp(v0, v1, sampleWeight);
                }
                else
                    v0 = xyz(idleSamples[i]);

                v0.store(outSamplePtr);
                stride = 3;
                break;
            }
            default: n_error("unhandled enum"); break;
        }

        if (curve.curveType == CurveType::Velocity)
        {
            outSamplePtr[0] = outSamplePtr[0] * velocityScale.x;
            outSamplePtr[1] = outSamplePtr[1] * velocityScale.y;
            outSamplePtr[2] = outSamplePtr[2] * velocityScale.z;
        }

        outSamplePtr += stride;

        *outSampleCounts = activeCurve;
        ++outSampleCounts;

        *lastUsedIntervalPtr = key;
        ++lastUsedIntervalPtr;
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimMix(
    const AnimClip& clip,
    const Util::FixedArray<AnimCurve>& curves,
    const AnimSampleMask* mask,
    float mixWeight,
    const float* src0SamplePtr,
    const float* src1SamplePtr,
    const uchar* src0SampleCounts,
    const uchar* src1SampleCounts,
    float* outSamplePtr,
    uchar* outSampleCounts)
{
    int i;
    for (i = 0; i < clip.numCurves; i++)
    {
        const AnimCurve& curve = curves[clip.firstCurve + i];
        const int stride = curve.curveType == CurveType::Rotation ? 4 : 3;

        // a joint has a translation, rotation and scale curve, joints the mask has no weight for aren't masked
        float weight = mixWeight;
        const IndexT joint = i / 3;
        if (mask != nullptr && joint < mask->weights.Size())
            weight *= mask->weights[joint];

        if (src1SampleCounts[i] == 0 || weight <= 0.0f)
        {
            // keep the samples mixed over
            for (int c = 0; c < stride; c++)
                outSamplePtr[c] = src0SamplePtr[c];
        }
        else if (src0SampleCounts[i] == 0 || weight >= 1.0f)
        {
            for (int c = 0; c < stride; c++)
                outSamplePtr[c] = src1SamplePtr[c];
        }
        else if (curve.curveType == CurveType::Rotation)
        {
            Math::quat q0, q1;
            q0.loadu(src0SamplePtr);
            q1.loadu(src1SamplePtr);
            Math::slerp(q0, q1, weight).storeu(outSamplePtr);
        }
        else
        {
            for (int c = 0; c < stride; c++)
                outSamplePtr[c] = Math::lerp(src0SamplePtr[c], src1SamplePtr[c], weight);
        }
        outSampleCounts[i] = src0SampleCounts[i] | src1SampleCounts[i];

        src0SamplePtr += stride;
        src1SamplePtr += stride;
        outSamplePtr += stride;
    }
}

} // namespace CoreAnimation
