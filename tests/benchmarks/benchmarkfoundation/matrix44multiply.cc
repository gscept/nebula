//------------------------------------------------------------------------------
//  matrix44multiply.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "matrix44multiply.h"
#include "math/mat4.h"
#include "math/vec4.h"

namespace Benchmarking
{
__ImplementClass(Benchmarking::Matrix44Multiply, 'M4ML', Benchmarking::Benchmark);

using namespace Timing;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
void
Matrix44Multiply::Run(Timer& timer)
{
    // setup some matrix44 arrays
    const int num = 100000;
    mat4* m0 = new mat4[num];
    mat4* m1 = new mat4[num];
    mat4* res = new mat4[num];
    mat4 m(vec4(1.0f, 2.0f, 3.0f, 0.0f),
        vec4(4.0f, 5.0f, 6.0f, 0.0f),
        vec4(7.0f, 8.0f, 9.0f, 0.0f),
        vec4(1.0f, 2.0f, 3.0f, 1.0f));
    IndexT i;
    for (i = 0; i < num; i++)
    {
        m0[i] = m;
    }
    for (i = 0; i < num; i++)
    {
        m1[i] = m;
    }
    
    mat4 tmp0;
    mat4 tmp1;
    mat4 tmp2;
    timer.Start();
    for (i = 0; i < 10; i++)
    {
        IndexT j;
        for (j = 0; j < num; j++)
        {
            tmp0 = m1[j] * m0[j];
            tmp1 = m0[j] * m1[j];
            tmp2 = tmp1 * tmp0;
            tmp0 = tmp2 * tmp1;
            tmp1 = tmp2 * tmp0;
            res[j] = tmp1 * tmp0;
        }
    }
    timer.Stop();
}

} // namespace Math
