//------------------------------------------------------------------------------
//  animjobsample.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "jobs/stdjob.h"
#include "animjobutil.h"
#include "coreanimation/animsamplemixinfo.h"
#include "coreanimation/sampletype.h"
#include "characters/characterjointmask.h"

namespace CoreAnimation
{

//------------------------------------------------------------------------------
/**
    This job function only performs sampling, not mixing. This is usually
    called for the first anim job of a mixing chain.
*/
void
AnimSampleJobFunc(const JobFuncContext& ctx)
{
    const AnimCurve* animCurves = (const AnimCurve*) ctx.uniforms[0];
    int numCurves = ctx.uniformSizes[0] / sizeof(AnimCurve);
    const AnimSampleMixInfo* info = (const AnimSampleMixInfo*) ctx.uniforms[1];
    const float4* src0SamplePtr = (const float4*) ctx.inputs[0];
    const float4* src1SamplePtr = (const float4*) ctx.inputs[1];
    float4* outSamplePtr = (float4*) ctx.outputs[0];
    uchar* outSampleCounts = ctx.outputs[1];

    if (SampleType::Step == info->sampleType)
    {
        AnimJobUtilSampleStep(animCurves, numCurves, info->velocityScale, src0SamplePtr, outSamplePtr, outSampleCounts);
    }
    else
    {
        AnimJobUtilSampleLinear(animCurves, numCurves, info->sampleWeight, info->velocityScale, src0SamplePtr, src1SamplePtr, outSamplePtr, outSampleCounts);
    }
}

} // namespace CoreAnimation
__ImplementSpursJob(CoreAnimation::AnimSampleJobFunc);
