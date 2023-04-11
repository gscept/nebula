//------------------------------------------------------------------------------
//  @file animtest.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "coreanimation/animkeybuffer.h"
#include "coreanimation/animcurve.h"
#include "timing/time.h"
#include "coreanimation/animation.h"
#include "animtest.h"
namespace Test
{

__ImplementClass(AnimTest, 'ANTE', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
void
AnimTest::Run()
{
    using namespace CoreAnimation;
    Ptr<AnimKeyBuffer> buffer = AnimKeyBuffer::Create();

    // The keys are, pos1, pos2, pos3, rot1, rot2, rot3, scale1, scale2, scale3...
    float keyBuffer[] = {
        0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
        , 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15
        , 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
    };

    AnimKeyBuffer::Interval intervalBuffer[] = {
        { 0, 24, 0, 1 }, { 24, 48, 1, 2}, { 48, 72, 2, 3 }
        , { 0, 24, 0, 1 }, { 24, 48, 1, 2}, { 48, 72, 2, 3 }
        , { 0, 24, 0, 1 }, { 24, 48, 1, 2}, { 48, 72, 2, 3 }
    };

    const SizeT numKeys = sizeof(keyBuffer) / sizeof(keyBuffer[0]);
    const SizeT numIntervals = sizeof(intervalBuffer) / sizeof(intervalBuffer[0]);
    buffer->Setup(numIntervals, numKeys, intervalBuffer, keyBuffer);
    Util::FixedArray<uint> sampleIndices(3, 0x0);

    // Setup curve
    AnimCurve posCurve;
    posCurve.firstIntervalOffset = 0;
    posCurve.numIntervals = 3;
    posCurve.curveType = CurveType::Translation;
    posCurve.postInfinityType = InfinityType::Cycle;
    posCurve.preInfinityType = InfinityType::Cycle;

    auto keyIntervalBuffer = buffer->GetIntervalBufferPointer();
    auto keySampleBuffer = buffer->GetKeyBufferPointer();
    auto sampleIndexBuffer = sampleIndices.Begin();

    uint key = 0;

    // Any sample between times 0-24 should give the first sample
    AnimKeyBuffer::Interval interval = FindNextInterval(posCurve, 23, key, keyIntervalBuffer);
    VERIFY(interval.start == 0);
    VERIFY(interval.end == 24);
    VERIFY(interval.key0 == 0);
    VERIFY(interval.key1 == 1);

    // Any sample between 25-48 should give key 1
    key = 0;
    interval = FindNextInterval(posCurve, 47, key, keyIntervalBuffer);
    VERIFY(interval.start == 24);
    VERIFY(interval.end == 48);
    VERIFY(interval.key0 == 1);
    VERIFY(interval.key1 == 2); 

    // Finding a key outside the range should just clamp it to the last one
    key = 0;
    interval = FindNextInterval(posCurve, 73, key, keyIntervalBuffer);
    VERIFY(interval.start == 48);
    VERIFY(interval.end == 72);
    VERIFY(interval.key0 == 2);
    VERIFY(interval.key1 == 3);

    AnimCurve nullScale;
    nullScale.firstIntervalOffset = 0;
    nullScale.numIntervals = 0;
    nullScale.curveType = CurveType::Scale;
    nullScale.postInfinityType = InfinityType::Cycle;
    nullScale.preInfinityType = InfinityType::Cycle;

    AnimCurve nullRotation;
    nullRotation.firstIntervalOffset = 0;
    nullRotation.numIntervals = 0;
    nullRotation.curveType = CurveType::Rotation;
    nullRotation.postInfinityType = InfinityType::Cycle;
    nullRotation.preInfinityType = InfinityType::Cycle;

    Util::FixedArray<Math::vec4> idleSamples = {
        Math::vec4(0, 0, 0, 0)
        , Math::vec4(1, 1, 1, 1)
        , Math::vec4(0, 0, 0, 1)
    };

    AnimClip clip;
    clip.firstCurve = 0;
    clip.numCurves = 3;

    // Use some dead curves for scale and rotation
    Util::FixedArray<AnimCurve> curves = { posCurve, nullScale, nullRotation };

    // Run anim sample with one active (pos) and two dead (scale rotation) curves
    float value[10];
    uchar count[3] = { 0, 0, 0 };
    AnimSampleStep(clip, curves, 0, Math::vec4{ 1 }, idleSamples, keySampleBuffer, keyIntervalBuffer, sampleIndexBuffer, value, count);
    VERIFY(value[0] == 0.0f);
    VERIFY(value[1] == 1.0f);
    VERIFY(value[2] == 2.0f);
    VERIFY(value[3] == 1.0f);
    VERIFY(value[4] == 1.0f);
    VERIFY(value[5] == 1.0f);
    VERIFY(value[6] == 0.0f);
    VERIFY(value[7] == 0.0f);
    VERIFY(value[8] == 0.0f);
    VERIFY(value[9] == 1.0f);

    // When we sample linear, we should get
    // 1.5 - lerp(0.0f, 1.0f, 0.5f);
    // 2.5f - lerp(1.0f, 2.0f, 1.5f);
    // 3.5f - lerp(2.0f, 3.0f, 2.5f);
    AnimSampleLinear(clip, curves, 12, Math::vec4{ 1 }, idleSamples, keySampleBuffer, keyIntervalBuffer, sampleIndexBuffer, value, count);
    VERIFY(value[0] == 0.5f);
    VERIFY(value[1] == 1.5f);
    VERIFY(value[2] == 2.5f);
    VERIFY(value[3] == 1.0f);
    VERIFY(value[4] == 1.0f);
    VERIFY(value[5] == 1.0f);
    VERIFY(value[6] == 0.0f);
    VERIFY(value[7] == 0.0f);
    VERIFY(value[8] == 0.0f);
    VERIFY(value[9] == 1.0f);
}

} // namespace Test
