//------------------------------------------------------------------------------
//  animsamplejob.cc
//  (C) 2018-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "jobs/jobs.h"
#include "coreanimation/animsamplemixinfo.h"
#include "animsamplemask.h"
#include "animcurve.h"
#include "profiling/profiling.h"

using namespace Math;
namespace CoreAnimation
{

//------------------------------------------------------------------------------
/**
*/
void
AnimSampleStep(const AnimCurve* curves,
	int numCurves,
	const float4& velocityScale,
	const float4* src0SamplePtr,
	float4* outSamplePtr,
	uchar* outSampleCounts)
{
	float4 f0;
	int i;
	for (i = 0; i < numCurves; i++)
	{
		const AnimCurve& curve = curves[i];
		if (!curve.IsActive())
		{
			// an inactive curve, set sample count to 0
			outSampleCounts[i] = 0;
		}
		else
		{
			// curve is active, set sample count to 1
			outSampleCounts[i] = 1;

			if (curve.IsStatic())
			{
				f0 = curve.GetStaticKey();
			}
			else
			{
				f0.load((scalar*)src0SamplePtr);
				src0SamplePtr++;
			}

			// if a velocity curve, multiply the velocity scale
			// (this is necessary if time factor is != 1)
			if (curve.GetCurveType() == CurveType::Velocity)
			{
				f0 = float4::multiply(f0, velocityScale);
			}
			f0.store((scalar*)outSamplePtr);
		}
		outSamplePtr++;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimSampleLinear(const AnimCurve* curves,
	int numCurves,
	float sampleWeight,
	const float4& velocityScale,
	const float4* src0SamplePtr,
	const float4* src1SamplePtr,
	float4* outSamplePtr,
	uchar* outSampleCounts)
{
	float4 f0, f1, fDst;
	quaternion q0, q1, qDst;
	int i;
	for (i = 0; i < numCurves; i++)
	{
		const AnimCurve& curve = curves[i];
		if (!curve.IsActive())
		{
			// an inactive curve, set sample count to 0
			outSampleCounts[i] = 0;
		}
		else
		{
			CurveType::Code curveType = curve.GetCurveType();

			// curve is active, set sample count to 1
			outSampleCounts[i] = 1;

			if (curve.IsStatic())
			{
				// a static curve, just copy the curve's static key as output
				f0 = curve.GetStaticKey();
				if (CurveType::Velocity == curveType)
				{
					f0 = float4::multiply(f0, velocityScale);
				}
				f0.store((scalar*)outSamplePtr);
			}
			else
			{
				if (curve.GetCurveType() == CurveType::Rotation)
				{
					q0.load((scalar*)src0SamplePtr);
					q1.load((scalar*)src1SamplePtr);
					qDst = quaternion::slerp(q0, q1, sampleWeight);
					qDst.store((scalar*)outSamplePtr);
				}
				else
				{
					f0.load((scalar*)src0SamplePtr);
					f1.load((scalar*)src1SamplePtr);
					fDst = float4::lerp(f0, f1, sampleWeight);
					if (CurveType::Velocity == curveType)
					{
						fDst = float4::multiply(fDst, velocityScale);
					}
					fDst.store((scalar*)outSamplePtr);
				}
				src0SamplePtr++;
				src1SamplePtr++;
			}
		}
		outSamplePtr++;
	}
}

//------------------------------------------------------------------------------
/**
*/
void 
AnimMix(const AnimCurve* curves,
	int numCurves,
	const AnimSampleMask* mask,
	float mixWeight,
	const float4* src0SamplePtr,
	const float4* src1SamplePtr,
	const uchar* src0SampleCounts,
	const uchar* src1SampleCounts,
	float4* outSamplePtr,
	uchar* outSampleCounts)
{
	float4 f0, f1, fDst;
	quaternion q0, q1, qDst;
	int i;
	for (i = 0; i < numCurves; i++)
	{
		const AnimCurve& curve = curves[i];
		uchar src0Count = src0SampleCounts[i];
		uchar src1Count = src1SampleCounts[i];

		// update dst sample counts
		outSampleCounts[i] = src0Count + src1Count;

		if ((src0Count > 0) && (src1Count > 0))
		{
			float maskWeight = 1;

			// we have 4 curves per joint
			if (mask != 0) maskWeight = mask->weights[i / 4];

			// both samples valid, perform normal mixing
			if (curve.GetCurveType() == CurveType::Rotation)
			{
				q0.load((scalar*)src0SamplePtr);
				q1.load((scalar*)src1SamplePtr);
				qDst = quaternion::slerp(q0, q1, mixWeight * maskWeight);
				qDst.store((scalar*)outSamplePtr);
			}
			else
			{
				f0.load((scalar*)src0SamplePtr);
				f1.load((scalar*)src1SamplePtr);
				fDst = float4::lerp(f0, f1, mixWeight * maskWeight);
				fDst.store((scalar*)outSamplePtr);
			}
		}
		else if (src0Count > 0)
		{
			// only "left" sample is valid
			f0.load((scalar*)src0SamplePtr);
			f0.store((scalar*)outSamplePtr);
		}
		else if (src1Count > 0)
		{
			// only "right" sample is valid
			f1.load((scalar*)src1SamplePtr);
			f1.store((scalar*)outSamplePtr);
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

//------------------------------------------------------------------------------
/**
*/
void
AnimSampleJob(const Jobs::JobFuncContext& ctx)
{
	N_SCOPE_ACCUM(AnimationSample, Animation);
	const AnimCurve* animCurves = (const AnimCurve*)ctx.uniforms[0];
	int numCurves = ctx.uniformSizes[0] / sizeof(AnimCurve);
	const AnimSampleMixInfo* info = (const AnimSampleMixInfo*)ctx.uniforms[1];
	for (ptrdiff sliceIdx = 0; sliceIdx < ctx.numSlices; sliceIdx++)
	{
		const float4* src0SamplePtr = (const float4*)N_JOB_INPUT(ctx, sliceIdx, 0);
		const float4* src1SamplePtr = (const float4*)N_JOB_INPUT(ctx, sliceIdx, 1);
		float4* outSamplePtr = (float4*)N_JOB_OUTPUT(ctx, sliceIdx, 0);
		uchar* outSampleCounts = N_JOB_OUTPUT(ctx, sliceIdx, 1);

		if (info->sampleType == SampleType::Step)
			AnimSampleStep(animCurves, numCurves, info->velocityScale, src0SamplePtr, outSamplePtr, outSampleCounts);
		else
			AnimSampleLinear(animCurves, numCurves, info->sampleWeight, info->velocityScale, src0SamplePtr, src1SamplePtr, outSamplePtr, outSampleCounts);
	}
}

//------------------------------------------------------------------------------
/**
*/
void
AnimSampleJobWithMix(const Jobs::JobFuncContext& ctx)
{
	N_SCOPE_ACCUM(AnimationSampleWithMix, Animation);
	const AnimCurve* animCurves = (const AnimCurve*)ctx.uniforms[0];
	int numCurves = ctx.uniformSizes[0] / sizeof(AnimCurve);
	const AnimSampleMixInfo* info = (const AnimSampleMixInfo*)ctx.uniforms[1];
	const AnimSampleMask* mask = (const AnimSampleMask*)ctx.uniforms[2];
	float4* tmpSamplePtr = (float4*)ctx.scratch;
	for (ptrdiff sliceIdx = 0; sliceIdx < ctx.numSlices; sliceIdx++)
	{
		const float4* src0SamplePtr = (const float4*)N_JOB_INPUT(ctx, sliceIdx, 0);
		const float4* src1SamplePtr = (const float4*)N_JOB_INPUT(ctx, sliceIdx, 1);
		const float4* mixSamplePtr = (const float4*)N_JOB_INPUT(ctx, sliceIdx, 2);
		uchar* mixSampleCounts = ctx.inputs[3] + sliceIdx * ctx.inputSizes[3];
		uchar* tmpSampleCounts = (uchar*)(tmpSamplePtr + numCurves);
		float4* outSamplePtr = (float4*)N_JOB_OUTPUT(ctx, sliceIdx, 0);
		uchar* outSampleCounts = N_JOB_OUTPUT(ctx, sliceIdx, 1);

		if (info->sampleType == SampleType::Step)
			AnimSampleStep(animCurves, numCurves, info->velocityScale, src0SamplePtr, tmpSamplePtr, tmpSampleCounts);
		else
			AnimSampleLinear(animCurves, numCurves, info->sampleWeight, info->velocityScale, src0SamplePtr, src1SamplePtr, tmpSamplePtr, tmpSampleCounts);

		AnimMix(animCurves, numCurves, mask, info->mixWeight, mixSamplePtr, tmpSamplePtr, mixSampleCounts, tmpSampleCounts, outSamplePtr, outSampleCounts);
	}
}

} // namespace CoreAnimation
