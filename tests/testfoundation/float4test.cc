//------------------------------------------------------------------------------
//  float4test.cc
//  (C) 2007 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "float4test.h"

namespace Test
{
__ImplementClass(Test::Float4Test, 'F4TS', Test::TestCase);

using namespace Math;

//------------------------------------------------------------------------------
/**
*/
void
Float4Test::Run()
{
    // construction
    vec4 v0(1.0f, 2.0f, 3.0f, 4.0f);
    vec4 v1(4.0f, 3.0f, 2.0f, 1.0f);
    vec4 v2(v0);
    vec4 v3(v1);
    VERIFY(v0 == v2);
    VERIFY(v1 == v3);
    VERIFY(v0 != v1);
    VERIFY(v2 != v3);
    VERIFY(v0 == vec4(1.0f, 2.0f, 3.0f, 4.0));

    // assignemt
    v2 = v1;
    VERIFY(v2 == v1);
    v2 = v0;
    VERIFY(v2 == v0);

    // operators
    v0 = -v0;
    VERIFY(v0 == vec4(-1.0f, -2.0f, -3.0f, -4.0f));
    v0 = -v0;
    VERIFY(v0 == v2);
    v2 += v3;
    VERIFY(v2 == vec4(5.0f, 5.0f, 5.0f, 5.0f));
    v2 -= v3;
    VERIFY(v2 == v0);
    v2 *= 2.0f;
    VERIFY(v2 == vec4(2.0f, 4.0f, 6.0f, 8.0f));
    v2 = v0 + v1;
    VERIFY(v2 == vec4(5.0f, 5.0f, 5.0f, 5.0f));
    v2 = v0 - v1;
    VERIFY(v2 == vec4(-3.0f, -1.0f, 1.0f, 3.0f));
    v2 = v0 * 2.0f;
    VERIFY(v2 == vec4(2.0f, 4.0f, 6.0f, 8.0f));

    // load and store
    NEBULA_ALIGN16 scalar f[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    NEBULA_ALIGN16 scalar f0[4];
    v2.load(f);
    VERIFY(v2 == vec4(1.0f, 2.0f, 3.0f, 4.0f));
    v2.loadu(f);
    VERIFY(v2 == vec4(1.0f, 2.0f, 3.0f, 4.0f));
    v2.store(f0);
    VERIFY((f0[0] == 1.0f) && (f0[1] == 2.0f) && (f0[2] == 3.0f) && (f0[3] == 4.0f));
    v2.storeu(f0);
    VERIFY((f0[0] == 1.0f) && (f0[1] == 2.0f) && (f0[2] == 3.0f) && (f0[3] == 4.0f));
    v2.stream(f0);
    VERIFY((f0[0] == 1.0f) && (f0[1] == 2.0f) && (f0[2] == 3.0f) && (f0[3] == 4.0f));

    // setting and getting content
    v2.set(2.0f, 3.0f, 4.0f, 5.0f);
    VERIFY(v2 == vec4(2.0f, 3.0f, 4.0f, 5.0f));
    VERIFY(v2.x == 2.0f);
    VERIFY(v2.y == 3.0f);
    VERIFY(v2.z == 4.0f);
    VERIFY(v2.w == 5.0f);
    v2.x = 1.0f;
    v2.y = 2.0f;
    v2.z = 3.0f;
    v2.w = 4.0f;
    VERIFY(v2 == vec4(1.0f, 2.0f, 3.0f, 4.0f));

    // length and abs
    v2.set(0.0f, 2.0f, 0.0f, 0.0f);
    VERIFY(fequal(length(v2), 2.0f, 0.0001f));
    VERIFY(fequal(lengthsq(v2), 4.0f, 0.0001f));
    v2.set(-1.0f, 2.0f, -3.0f, 4.0f);
    VERIFY(abs(v2) == vec4(1.0f, 2.0f, 3.0f, 4.0f));
    
    // cross3
    v0.set(1.0f, 0.0f, 0.0f, 0.0f);
    v1.set(0.0f, 0.0f, 1.0f, 0.0f);
    v2 = cross3(v0, v1);
    VERIFY(v2 == vec4(0.0f, -1.0f, 0.0f, 0.0f));

    // dot3
    v0.set(1.0f, 0.0f, 0.0f, 0.0f);
    v1.set(1.0f, 0.0f, 0.0f, 0.0f);    
    VERIFY(dot3(v0, v1) == 1.0f);
    v1.set(-1.0f, 0.0f, 0.0f, 0.0f);
    VERIFY(dot3(v0, v1) == -1.0f);
    v1.set(0.0f, 1.0f, 0.0f, 0.0f);
    VERIFY(dot3(v0, v1) == 0.0f);

    // @todo: test barycentric(), catmullrom(), hermite()

    // lerp
    v0.set(1.0f, 2.0f, 3.0f, 4.0f);
    v1.set(2.0f, 3.0f, 4.0f, 5.0f);
    v2 = lerp(v0, v1, 0.5f);
    VERIFY(v2 == vec4(1.5f, 2.5f, 3.5f, 4.5f));
    
    // maximize/minimize
    v0.set(1.0f, 2.0f, 3.0f, 4.0f);
    v1.set(4.0f, 3.0f, 2.0f, 1.0f);
    v2 = maximize(v0, v1);
    VERIFY(v2 == vec4(4.0f, 3.0f, 3.0f, 4.0f));
    v2 = minimize(v0, v1);
    VERIFY(v2 == vec4(1.0f, 2.0f, 2.0f, 1.0f));

    // normalize
    v0.set(2.5f, 0.0f, 0.0f, 0.0f);
    v1 = normalize(v0);
    VERIFY(v1 == vec4(1.0f, 0.0f, 0.0f, 0.0f));

    // transform (point and vector)
    mat4 m = translation(1.0f, 2.0f, 3.0f);
    v0.set(1.0f, 0.0f, 0.0f, 1.0f);
    v1 = m * v0;
    VERIFY(v1 == vec4(2.0f, 2.0f, 3.0f, 1.0f));
    v0.set(1.0f, 0.0f, 0.0f, 0.0f);
    v1 = m * v0;
    VERIFY(v0 == vec4(1.0f, 0.0f, 0.0f, 0.0f));

    // component-wise comparison
    v0.set(1.0f, 1.0f, 1.0f, 1.0f);
    v1.set(0.5f, 1.5f, 0.5f, 1.5f);
    v2.set(2.0f, 2.0f, 2.0f, 2.0f);
    VERIFY(less_any(v0, v1));
    VERIFY(greater_any(v0, v1));
    VERIFY(!less_all(v0, v1));
    VERIFY(!greater_all(v0, v1));
    VERIFY(lessequal_all(v0, v2));
    VERIFY(greaterequal_all(v2, v0));
}

}