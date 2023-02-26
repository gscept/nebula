//------------------------------------------------------------------------------
//  animsamplejob.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "jobs/jobs.h"
#include "coreanimation/animsamplemixinfo.h"
#include "animsamplemask.h"
#include "animkeybuffer.h"
#include "animcurve.h"
#include "animclip.h"
#include "profiling/profiling.h"

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
inline AnimKeyBuffer::Interval
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
        int idleSampleIndex = i / 3;

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
                    idleSamples[i].store(outSamplePtr);
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
                    idleSamples[i].store(outSamplePtr);
                stride = 3;
                break;
            }
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
        int idleSampleIndex = i / 3;

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
                sampleWeight = (wrappedTime - currentTime.start) / float(currentTime.end - currentTime.start);
        }

        switch (curve.curveType)
        {
            case CurveType::Rotation:
            {
                Math::quat q0;
                if (activeCurve)
                {
                    Math::quat q1;
                    q0.load(&srcSamplePtr[currentTime.key0]);
                    q1.load(&srcSamplePtr[currentTime.key1]);
                    q0 = Math::slerp(q0, q1, sampleWeight);
                }
                else
                    q0 = idleSamples[i].vec;

                q0.store(outSamplePtr);
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
                    v0.load(&srcSamplePtr[currentTime.key0]);
                    v1.load(&srcSamplePtr[currentTime.key1]);
                    v0 = lerp(v0, v1, sampleWeight);
                }
                else
                    v0 = xyz(idleSamples[i]);

                v0.store(outSamplePtr);
                stride = 3;
                break;
            }
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
    const SizeT numSamples,
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
    for (i = 0; i < numSamples; i++)
    {
        uchar src0Count = src0SampleCounts[i];
        uchar src1Count = src1SampleCounts[i];

        // update dst sample counts
        outSampleCounts[i] = src0Count + src1Count;

        if ((src0Count > 0) && (src1Count > 0))
        {
            float maskWeight = 1;

            // we have 4 curves per joint
            if (mask != 0) maskWeight = mask->weights[i / 4];

            *outSamplePtr = lerp(*src0SamplePtr, *src1SamplePtr, mixWeight * maskWeight);
        }
        else if (src0Count > 0)
        {
            // only "left" sample is valid
            *outSamplePtr = *src0SamplePtr;
        }
        else if (src1Count > 0)
        {
            // only "right" sample is valid
            *outSamplePtr = *src1SamplePtr;
        }
        else
        {
            // neither the left nor the right sample is valid,
            // sample key is undefined
        }

        // update pointers
        src0SamplePtr++;
        src1SamplePtr++;
        outSamplePtr++;
    }
}

} // namespace CoreAnimation
