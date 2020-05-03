//------------------------------------------------------------------------------
//  win360_testjob.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "jobs/stdjob.h"
#include "math/float4.h"

namespace Test
{
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
void
TestJobFunc(const JobFuncContext& ctx)
{
    scalar* src = (scalar*) ctx.inputs[0];
    scalar* dst = (scalar*) ctx.outputs[0];
    SizeT numElements = ctx.inputSizes[0] / sizeof(float4);
    float4 srcVec, dstVec;
    float4 addVec(1.0f, 2.0f, 3.0f, 0.0f);
    IndexT i;
    for (i = 0; i < numElements; i++)
    {
        srcVec.load(src);
        dstVec = srcVec + addVec;
        dstVec.stream(dst);
        src += 4;
        dst += 4;
    }
}

}