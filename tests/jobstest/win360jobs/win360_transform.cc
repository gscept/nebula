//------------------------------------------------------------------------------
//  win360job_transform.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "jobs/stdjob.h"
#include "math/float4.h"
#include "math/matrix44.h"

namespace Test
{
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
void
TransformJobFunc(const JobFuncContext& ctx)
{
    matrix44 m;
    m.load((scalar*)ctx.uniforms[0]);
    scalar* src = (scalar*) ctx.inputs[0];
    scalar* dst = (scalar*) ctx.outputs[0];
    SizeT numElements = ctx.inputSizes[0] / sizeof(float4);
    float4 srcVec, dstVec;
    IndexT i;
    for (i = 0; i < numElements; i++)
    {
        srcVec.load(src);
        dstVec = float4::transform(srcVec, m);
        dstVec.stream(dst);
        src += 4;
        dst += 4;
    }
}

}