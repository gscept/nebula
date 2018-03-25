//------------------------------------------------------------------------------
//  animutil.cc
//  (C) 2008 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coreanimation/animutil.h"
#include "coreanimation/animsamplemixinfo.h"
#include "math/quaternion.h"
#include "util/round.h"

namespace CoreAnimation
{
using namespace Math;
using namespace Timing;
using namespace Util;
using namespace Jobs;

// job function declaration
#if __PS3__
extern "C" {
    extern const char _binary_jqjob_render_animjobsample_ps3_bin_start[];
    extern const char _binary_jqjob_render_animjobsample_ps3_bin_size[];
    extern const char _binary_jqjob_render_animjobsamplemix_ps3_bin_start[];
    extern const char _binary_jqjob_render_animjobsamplemix_ps3_bin_size[];
}
#else
extern void AnimSampleJobFunc(const JobFuncContext& ctx);
extern void AnimSampleMixJobFunc(const JobFuncContext& ctx);
#endif

//------------------------------------------------------------------------------
/**
    Clamp key indices into the valid range, take pre-infinity and 
    post-infinity type into account.
*/
IndexT
AnimUtil::ClampKeyIndex(IndexT keyIndex, const AnimClip& clip)
{
    SizeT clipNumKeys = clip.GetNumKeys();
    if (keyIndex < 0)
    {
        if (clip.GetPreInfinityType() == InfinityType::Cycle)
        {
            keyIndex %= clipNumKeys;
            if (keyIndex < 0)
            {
                keyIndex += clipNumKeys;
            }
        }
        else
        {
            keyIndex = 0;
        }
    }
    else if (keyIndex >= clipNumKeys)
    {
        if (clip.GetPostInfinityType() == InfinityType::Cycle)
        {
            keyIndex %= clipNumKeys;
        }
        else
        {
            keyIndex = clipNumKeys - 1;
        }
    }
    return keyIndex;
}

//------------------------------------------------------------------------------
/**
    Compute the inbetween-ticks between two frames for a given sample time.
*/
Timing::Tick
AnimUtil::InbetweenTicks(Timing::Tick sampleTime, const AnimClip& clip)
{
    // normalize sample time into valid time range
    Timing::Tick clipDuration = clip.GetClipDuration();
    if (sampleTime < 0)
    {
        if (clip.GetPreInfinityType() == InfinityType::Cycle)
        {
            sampleTime %= clipDuration;
            if (sampleTime < 0)
            {
                sampleTime += clipDuration;
            }
        }
        else
        {
            sampleTime = 0;
        }
    }
    else if (sampleTime >= clipDuration)
    {
        if (clip.GetPostInfinityType() == InfinityType::Cycle)
        {
            sampleTime %= clipDuration;
        }
        else
        {
            sampleTime = clipDuration;
        }
    }

    Timing::Tick inbetweenTicks = sampleTime % clip.GetKeyDuration();
    return inbetweenTicks;
}

//------------------------------------------------------------------------------
/**
    NOTE: this method is obsolete
    NOTE: The sampler will *NOT* the start time of the clip into account!
    TODO: seperate delta computation from default sampling, set curveindex from jointname in characterinstance !!!
*/
void
AnimUtil::Sample(const AnimResourceId& animResource,
                 IndexT clipIndex, 
                 SampleType::Code sampleType, 
                 Tick time, 
                 float timeFactor,
                 const Ptr<AnimSampleBuffer>& result)
{
    n_assert(animResource.isvalid());
    n_assert(result.isvalid());
    n_assert((sampleType == SampleType::Step) || (sampleType == SampleType::Linear));
 
    const AnimClip& clip = animResource->GetClipByIndex(clipIndex);
    SizeT keyStride  = clip.GetKeyStride();
    Tick keyDuration = clip.GetKeyDuration();
    n_assert(clip.GetNumCurves() == result->GetNumSamples());

    // compute the relative first key index, second key index and lerp value
    IndexT keyIndex0 = AnimUtil::ClampKeyIndex((time / keyDuration), clip);
    IndexT keyIndex1 = AnimUtil::ClampKeyIndex(keyIndex0 + 1, clip);
    Tick inbetweenTicks = AnimUtil::InbetweenTicks(time, clip);
    float lerpValue = float(inbetweenTicks) / float(keyDuration);  

    // sample curves...
    float4 f4Key0, f4Key1, f4SampleKey;
    quaternion qKey0, qKey1, qSampleKey;
    float4 vecOne(1.0f, 1.0f, 1.0f, 1.0f);
    float4 scaleVec;

    float4* srcKeyStart = animResource->GetKeyBuffer()->GetKeyBufferPointer();
    float4* dstKeyBuffer = result->GetSamplesPointer();
    uchar* dstSampleCounts = result->GetSampleCountsPointer();
    IndexT curveIndex;
    SizeT numCurves = clip.GetNumCurves();
    for (curveIndex = 0; curveIndex < numCurves; curveIndex++)
    {
        const AnimCurve& curve = clip.CurveByIndex(curveIndex);
        if (!curve.IsActive())
        {
            // curve is not active, set sample count to 0
            dstSampleCounts[curveIndex] = 0;            
        }
        else
        {
            if (curve.GetCurveType() == CurveType::Velocity)
            {
                scaleVec.set(timeFactor, timeFactor, timeFactor, 1.0f);
            }
            else
            {
                scaleVec = vecOne;
            }

            // curve is active, set sample count to 1
            dstSampleCounts[curveIndex] = 1;
            scalar* resultPtr = (scalar*) &(dstKeyBuffer[curveIndex]);
            if (curve.IsStatic())
            {
                // the curve is constant, just copy the constant value into the result buffer
                float4 val = float4::multiply(curve.GetStaticKey(), scaleVec);
                val.store(resultPtr);
            }
            else
            {
                // non-static curve, need to actually sample the result
                IndexT startKeyIndex = curve.GetFirstKeyIndex();

                // compute position of first key in source key buffer
                scalar* keyPtr0 = (scalar*) (srcKeyStart + startKeyIndex + (keyIndex0 * keyStride));
                if (SampleType::Step == sampleType)
                {
                    // if no interpolation needed, just copy the key to the sample buffer
                    f4SampleKey.load(keyPtr0);
                    f4SampleKey = float4::multiply(f4SampleKey, scaleVec);
                    f4SampleKey.store(resultPtr);
                }
                else
                {
                    // need to interpolate between 2 keys
                    scalar* keyPtr1 = (scalar*) (srcKeyStart + startKeyIndex + (keyIndex1 * keyStride));
                    if (curve.GetCurveType() == CurveType::Rotation)
                    {
                        // perform spherical interpolation
                        qKey0.load(keyPtr0);
                        qKey1.load(keyPtr1);
                        qSampleKey = quaternion::slerp(qKey0, qKey1, lerpValue);
                        qSampleKey.store(resultPtr);
                    }
                    else
                    {
                        // perform linear interpolation
                        f4Key0.load(keyPtr0);
                        f4Key1.load(keyPtr1);
                        f4SampleKey = float4::multiply(float4::lerp(f4Key0, f4Key1, lerpValue), scaleVec);
                        f4SampleKey.store(resultPtr);
                    }
                }
            }            
        }
    }
}

//------------------------------------------------------------------------------
/**
    Create a job object which is setup to perform simple animation sampling.
*/
Ptr<Job>
AnimUtil::CreateSampleJob(const AnimResourceId& animResource,
                          IndexT clipIndex, 
                          SampleType::Code sampleType, 
                          Timing::Tick time, 
                          float timeFactor, 
						  const Characters::CharacterJointMask* mask,
                          const Ptr<AnimSampleBuffer>& resultBuffer)
{
    n_assert(animResource->GetClipByIndex(0).GetNumCurves() == resultBuffer->GetNumSamples());

    // create a job object
    Ptr<Job> job = Job::Create();

    // need to compute the sample weight and pointers to "before" and "after" keys
    const AnimClip& clip = animResource->GetClipByIndex(clipIndex);
    Tick keyDuration = clip.GetKeyDuration();
    IndexT keyIndex0 = AnimUtil::ClampKeyIndex((time / keyDuration), clip);
    IndexT keyIndex1 = AnimUtil::ClampKeyIndex(keyIndex0 + 1, clip);
    Tick inbetweenTicks = AnimUtil::InbetweenTicks(time, clip);

    // allocate AnimSampleMixInfo as buffer owned by job
    AnimSampleMixInfo* sampleMixInfo = (AnimSampleMixInfo*) job->AllocPrivateBuffer(Memory::DefaultHeap, sizeof(AnimSampleMixInfo));
    Memory::Clear(sampleMixInfo, sizeof(AnimSampleMixInfo));
    sampleMixInfo->sampleType = sampleType;
    sampleMixInfo->sampleWeight = float(inbetweenTicks) / float(keyDuration);
    sampleMixInfo->velocityScale.set(timeFactor, timeFactor, timeFactor, 1.0f);

    // get start pointers to source keys
    SizeT src0ByteSize, src1ByteSize;
    const float4* src0SamplePtr = animResource->ComputeKeySlicePointerAndSize(clipIndex, keyIndex0, src0ByteSize);
    const float4* src1SamplePtr = animResource->ComputeKeySlicePointerAndSize(clipIndex, keyIndex1, src1ByteSize);

    // get pointers to output buffers
    float4* outSamplesPtr = resultBuffer->GetSamplesPointer();
    SizeT numOutSamples = resultBuffer->GetNumSamples();
    SizeT outSamplesByteSize = numOutSamples * sizeof(float4);
    uchar* outSampleCounts = resultBuffer->GetSampleCountsPointer();

    // setup job data descriptors
    JobDataDesc inputs((void*)src0SamplePtr, src0ByteSize, src0ByteSize,
                       (void*)src1SamplePtr, src1ByteSize, src1ByteSize);
    JobDataDesc outputs(outSamplesPtr, outSamplesByteSize, outSamplesByteSize,
                        outSampleCounts, Round::RoundUp16(numOutSamples), Round::RoundUp16(numOutSamples));
    JobUniformDesc uniforms(&(clip.CurveByIndex(0)), clip.GetNumCurves() * sizeof(AnimCurve),
                            sampleMixInfo, sizeof(AnimSampleMixInfo), 
							(void*)mask, sizeof(Characters::CharacterJointMask),
							0);

    #if __PS3__
    JobFuncDesc jobFuncDesc(_binary_jqjob_render_animjobsample_ps3_bin_start, _binary_jqjob_render_animjobsample_ps3_bin_size);
    #else
    JobFuncDesc jobFuncDesc(AnimSampleJobFunc);
    #endif

    job->Setup(uniforms, inputs, outputs, jobFuncDesc);

    return job;
}

//------------------------------------------------------------------------------
/**
    Create a job which performs both sampling and mixing.
*/
Ptr<Job>
AnimUtil::CreateSampleAndMixJob(const AnimResourceId& animResource,
                                IndexT clipIndex, 
                                SampleType::Code sampleType, 
                                Timing::Tick time, 
                                float timeFactor,
								const Characters::CharacterJointMask* mask,
                                float mixWeight, 
                                const Ptr<AnimSampleBuffer>& mixIn, 
                                const Ptr<AnimSampleBuffer>& resultBuffer)
{
    n_assert(animResource->GetClipByIndex(0).GetNumCurves() == resultBuffer->GetNumSamples());
    n_assert(mixIn->GetNumSamples() == resultBuffer->GetNumSamples());

    // create a job object
    Ptr<Job> job = Job::Create();

    // need to compute the sample weight and pointers to "before" and "after" keys
    const AnimClip& clip = animResource->GetClipByIndex(clipIndex);
    Tick keyDuration = clip.GetKeyDuration();
    IndexT keyIndex0 = AnimUtil::ClampKeyIndex((time / keyDuration), clip);
    IndexT keyIndex1 = AnimUtil::ClampKeyIndex(keyIndex0 + 1, clip);
    Tick inbetweenTicks = AnimUtil::InbetweenTicks(time, clip);

    // allocate AnimSampleMixInfo as buffer owned by job
    AnimSampleMixInfo* sampleMixInfo = (AnimSampleMixInfo*) job->AllocPrivateBuffer(Memory::DefaultHeap, sizeof(AnimSampleMixInfo));
    Memory::Clear(sampleMixInfo, sizeof(AnimSampleMixInfo));
    sampleMixInfo->sampleType = sampleType;
    sampleMixInfo->sampleWeight = float(inbetweenTicks) / float(keyDuration);
    sampleMixInfo->velocityScale.set(timeFactor, timeFactor, timeFactor, 1.0f);
    sampleMixInfo->mixWeight = mixWeight;

    // get start pointers to source keys
    SizeT src0ByteSize, src1ByteSize;
    const float4* src0SamplePtr = animResource->ComputeKeySlicePointerAndSize(clipIndex, keyIndex0, src0ByteSize);
    const float4* src1SamplePtr = animResource->ComputeKeySlicePointerAndSize(clipIndex, keyIndex1, src1ByteSize);
    const float4* mixSamplePtr = mixIn->GetSamplesPointer();
    uchar* mixSampleCounts = mixIn->GetSampleCountsPointer();
    SizeT mixNumSamples = mixIn->GetNumSamples();
    SizeT mixSamplesByteSize = mixNumSamples * sizeof(float4);

    // get pointers to output buffers
    float4* outSamplesPtr = resultBuffer->GetSamplesPointer();
    SizeT numOutSamples = resultBuffer->GetNumSamples();
    SizeT outSamplesByteSize = numOutSamples * sizeof(float4);
    uchar* outSampleCounts = resultBuffer->GetSampleCountsPointer();

    // scratch size must be big enough to hold one complete set of 
    // samples and sample counts
    SizeT scratchSize = outSamplesByteSize + numOutSamples;

    // setup job data descriptors
    JobDataDesc inputs((void*)src0SamplePtr, src0ByteSize, src0ByteSize,
                       (void*)src1SamplePtr, src1ByteSize, src1ByteSize,
                       (void*)mixSamplePtr, mixSamplesByteSize, mixSamplesByteSize,
                       (void*)mixSampleCounts, Round::RoundUp16(mixNumSamples), Round::RoundUp16(mixNumSamples));
    JobDataDesc outputs(outSamplesPtr, outSamplesByteSize, outSamplesByteSize,
                        outSampleCounts, Round::RoundUp16(numOutSamples), Round::RoundUp16(numOutSamples));
    JobUniformDesc uniforms(&(clip.CurveByIndex(0)), clip.GetNumCurves() * sizeof(AnimCurve),
                            sampleMixInfo, sizeof(AnimSampleMixInfo), 
							(void*)mask, sizeof(Characters::CharacterJointMask),
							scratchSize);

    #if __PS3__
    JobFuncDesc jobFuncDesc(_binary_jqjob_render_animjobsamplemix_ps3_bin_start, _binary_jqjob_render_animjobsamplemix_ps3_bin_size);
    #else
    JobFuncDesc jobFuncDesc(AnimSampleMixJobFunc);
    #endif

    job->Setup(uniforms, inputs, outputs, jobFuncDesc);

    return job;
}

} // namespace CoreAnimation