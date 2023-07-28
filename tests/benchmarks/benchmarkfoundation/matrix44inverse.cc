//------------------------------------------------------------------------------
//  matrix44inverse.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "matrix44inverse.h"

namespace Benchmarking
{
__ImplementClass(Benchmarking::Matrix44Inverse, 'M4IN', Benchmarking::Benchmark);

using namespace Timing;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
void
Matrix44Inverse::Run(Timer& timer)
{
    // setup some matrix44 arrays
    const int num = 100000;
    mat4* m0 = new mat4[num];
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
    
    timer.Start();
    for (i = 0; i < 10; i++)
    {
        IndexT j;
        for (j = 0; j < num; j++)
        {
            res[j] = inverse(m0[j]);
        }
    }
    timer.Stop();
    delete[] m0;
    delete[] res;
}

} // namespace Benchmarking
