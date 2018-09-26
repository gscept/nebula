//------------------------------------------------------------------------------
//  matrix44multiply.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "matrix44multiply.h"

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
    matrix44* m0 = n_new_array(matrix44, num);
    matrix44* m1 = n_new_array(matrix44, num);
    matrix44* res = n_new_array(matrix44, num);
    matrix44 m(float4(1.0f, 2.0f, 3.0f, 0.0f),
               float4(4.0f, 5.0f, 6.0f, 0.0f),
               float4(7.0f, 8.0f, 9.0f, 0.0f),
               float4(1.0f, 2.0f, 3.0f, 1.0f));
    IndexT i;
    for (i = 0; i < num; i++)
    {
        m0[i] = m;
    }
    for (i = 0; i < num; i++)
    {
        m1[i] = m;
    }
    
    matrix44 tmp0;
    matrix44 tmp1;
    matrix44 tmp2;
    timer.Start();
    for (i = 0; i < 10; i++)
    {
        IndexT j;
        for (j = 0; j < num; j++)
        {
            tmp0 = matrix44::multiply(m0[j], m1[j]);
            tmp1 = matrix44::multiply(m1[j], m0[j]);
            tmp2 = matrix44::multiply(tmp0, tmp1);
            tmp0 = matrix44::multiply(tmp1, tmp2);
            tmp1 = matrix44::multiply(tmp0, tmp2);
            res[j] = matrix44::multiply(tmp0, tmp1);
        }
    }
    timer.Stop();
}

} // namespace Math
