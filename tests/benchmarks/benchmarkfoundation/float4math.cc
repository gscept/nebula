//------------------------------------------------------------------------------
//  float4math.cc
//  (C) 2006 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "float4math.h"

namespace Benchmarking
{
__ImplementClass(Benchmarking::Float4Math, 'F4MT', Benchmarking::Benchmark);

using namespace Timing;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
void
Float4Math::Run(Timer& timer)
{
    // setup some float4 arrays
    const int num = 100000;
    vec4* f0 = n_new_array(vec4, num);
    vec4* f1 = n_new_array(vec4, num);
    vec4* res = n_new_array(vec4, num);
    vec4 v0(1.0f, 2.0f, 3.0f, 0.0f);
    vec4 v1(1.5f, 1.7f, 1.8f, 0.0f);
    vec4 v2(0.1f, 0.2f, 0.3f, 0.0f);
    vec4 v3(0.3f, 0.4f, 0.5f, 0.0f);
    IndexT i;
    for (i = 0; i < num; i++)
    {
        f0[i] = v0;
        v0 += v2;
    }
    for (i = 0; i < num; i++)
    {
        f1[i] = v1;
        v1 += v3;
    }

    vec4 tmp0;
    vec4 tmp1;
    vec4 tmp2;
    timer.Start();
    for (i = 0; i < 100; i++)
    {
        IndexT j;
        for (j = 0; j < num; j++)
        {
            // do a mix of math operations
            tmp0 = f0[j] + f1[j];
            tmp1 = tmp0 * dot3(f0[j], f1[j]);
            tmp2 = cross3(tmp0, tmp1);
            tmp1 = vecLerp(tmp0, tmp2, 0.5f);
            tmp0 = normalize(tmp1);
            tmp1 = maximize(tmp0, tmp1);
            tmp2 = minimize(tmp0, tmp1);
            res[j] = tmp1 - tmp2;
        }
    }
    timer.Stop();
    n_delete_array(f0);
    n_delete_array(f1);
    n_delete_array(res);
}

} // namespace Benchmarking
