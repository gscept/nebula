#pragma once
//------------------------------------------------------------------------------
/**
    @file animjobutil.h
    
    Utility functions for CoreAnimation subsystem jobs.
    
    (C) 2009 Radon Labs GmbH
*/
#include "math/float4.h"
#include "math/quaternion.h"
#include "coreanimation/animcurve.h"
#include "characters/characterjointmask.h"

namespace CoreAnimation
{

using namespace Math;

//------------------------------------------------------------------------------
/**
    Sampler for "step" interpolation type.
*/
inline void
AnimJobUtilSampleStep(const AnimCurve* curves,
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
    Sampler for "linear" interpolation type.
*/
inline void
AnimJobUtilSampleLinear(const AnimCurve* curves,
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
    Mixes 2 source sample buffers into a destination sample buffer using a
    single lerp-value between 0.0 and 1.0. Mixing takes sample counts
    into consideration. A source sample count of 0 indicates, the this
    sample is not valid and the result is made of 100% of the other sample.
    If both source samples are valid, the result is blended from both
    source samples. This gives the expected results when an animation clip
    only manipulates parts of a character skeleton.

    NOTE: the output data blocks may be identical with one of the
    input data blocks!
*/
inline void
AnimJobUtilMix(const AnimCurve* curves,
               int numCurves,
               const Characters::CharacterJointMask* mask,
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
            if (mask != 0) maskWeight = mask->GetWeight(i/4);

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

} // namespace CoreAnimation
