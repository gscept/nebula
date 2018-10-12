//------------------------------------------------------------------------------
//  animjobsamplemix.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "jobs/stdjob.h"
#include "animjobutil.h"
#include "coreanimation/animsamplemixinfo.h"
#include "characters/characterjointmask.h"

namespace CoreAnimation
{

//------------------------------------------------------------------------------
/**
    Performs both animation sampling, and mixing with another sample
    buffer in a single job.
*/
void
AnimSampleMixJobFunc(const JobFuncContext& ctx)
{
    const AnimCurve* animCurves = (const AnimCurve*) ctx.uniforms[0];
    int numCurves = ctx.uniformSizes[0] / sizeof(AnimCurve);
    const AnimSampleMixInfo* info = (const AnimSampleMixInfo*) ctx.uniforms[1];
	const Characters::CharacterJointMask* mask = (const Characters::CharacterJointMask*)ctx.uniforms[2];
    const float4* src0SamplePtr = (const float4*) ctx.inputs[0];
    const float4* src1SamplePtr = (const float4*) ctx.inputs[1];
    const float4* mixSamplePtr  = (const float4*) ctx.inputs[2];
    float4* tmpSamplePtr   = (float4*) ctx.scratch;
    uchar* tmpSampleCounts = (uchar*) (tmpSamplePtr + numCurves);
    uchar* mixSampleCounts = ctx.inputs[3];
    float4* outSamplePtr = (float4*) ctx.outputs[0];
    uchar* outSampleCounts = ctx.outputs[1];

    // first perform sampling step...
    if (SampleType::Step == info->sampleType)
    {
        AnimJobUtilSampleStep(animCurves, numCurves, info->velocityScale, src0SamplePtr, tmpSamplePtr, tmpSampleCounts);
    }
    else
    {
        AnimJobUtilSampleLinear(animCurves, numCurves, info->sampleWeight, info->velocityScale, src0SamplePtr, src1SamplePtr, tmpSamplePtr, tmpSampleCounts);
    }
    
    // ...then mixing, NOTE: outSamplePtr and outSampleCounts are read and overwritten!
    AnimJobUtilMix(animCurves, numCurves, mask, info->mixWeight, mixSamplePtr, tmpSamplePtr, mixSampleCounts, tmpSampleCounts, outSamplePtr, outSampleCounts);
}

} // namespace CoreAnimation
__ImplementSpursJob(CoreAnimation::AnimSampleMixJobFunc);
