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
    float4 v0(1.0f, 2.0f, 3.0f, 4.0f);
    float4 v1(4.0f, 3.0f, 2.0f, 1.0f);
    float4 v2(v0);
    float4 v3(v1);
    this->Verify(v0 == v2);
    this->Verify(v1 == v3);
    this->Verify(v0 != v1);
    this->Verify(v2 != v3);
    this->Verify(v0 == float4(1.0f, 2.0f, 3.0f, 4.0));

    // assignemt
    v2 = v1;
    this->Verify(v2 == v1);
    v2 = v0;
    this->Verify(v2 == v0);

    // operators
    v0 = -v0;
    this->Verify(v0 == float4(-1.0f, -2.0f, -3.0f, -4.0f));
    v0 = -v0;
    this->Verify(v0 == v2);
    v2 += v3;
    this->Verify(v2 == float4(5.0f, 5.0f, 5.0f, 5.0f));
    v2 -= v3;
    this->Verify(v2 == v0);
    v2 *= 2.0f;
    this->Verify(v2 == float4(2.0f, 4.0f, 6.0f, 8.0f));
    v2 = v0 + v1;
    this->Verify(v2 == float4(5.0f, 5.0f, 5.0f, 5.0f));
    v2 = v0 - v1;
    this->Verify(v2 == float4(-3.0f, -1.0f, 1.0f, 3.0f));
    v2 = v0 * 2.0f;
    this->Verify(v2 == float4(2.0f, 4.0f, 6.0f, 8.0f));

    // load and store
    NEBULA3_ALIGN16 scalar f[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    NEBULA3_ALIGN16 scalar f0[4];
    v2.load(f);
    this->Verify(v2 == float4(1.0f, 2.0f, 3.0f, 4.0f));
    v2.loadu(f);
    this->Verify(v2 == float4(1.0f, 2.0f, 3.0f, 4.0f));
    v2.store(f0);
    this->Verify((f0[0] == 1.0f) && (f0[1] == 2.0f) && (f0[2] == 3.0f) && (f0[3] == 4.0f));
    v2.storeu(f0);
    this->Verify((f0[0] == 1.0f) && (f0[1] == 2.0f) && (f0[2] == 3.0f) && (f0[3] == 4.0f));
    v2.stream(f0);
    this->Verify((f0[0] == 1.0f) && (f0[1] == 2.0f) && (f0[2] == 3.0f) && (f0[3] == 4.0f));

    // setting and getting content
    v2.set(2.0f, 3.0f, 4.0f, 5.0f);
    this->Verify(v2 == float4(2.0f, 3.0f, 4.0f, 5.0f));
    this->Verify(v2.x() == 2.0f);
    this->Verify(v2.y() == 3.0f);
    this->Verify(v2.z() == 4.0f);
    this->Verify(v2.w() == 5.0f);
    v2.x() = 1.0f;
    v2.y() = 2.0f;
    v2.z() = 3.0f;
    v2.w() = 4.0f;
    this->Verify(v2 == float4(1.0f, 2.0f, 3.0f, 4.0f));

    // length and abs
    v2.set(0.0f, 2.0f, 0.0f, 0.0f);
    this->Verify(n_fequal(v2.length(), 2.0f, 0.0001f));
    this->Verify(n_fequal(v2.lengthsq(), 4.0f, 0.0001f));
    v2.set(-1.0f, 2.0f, -3.0f, 4.0f);
    this->Verify(v2.abs() == float4(1.0f, 2.0f, 3.0f, 4.0f));
    
    // cross3
    v0.set(1.0f, 0.0f, 0.0f, 0.0f);
    v1.set(0.0f, 0.0f, 1.0f, 0.0f);
    v2 = float4::cross3(v0, v1);
    this->Verify(v2 == float4(0.0f, -1.0f, 0.0f, 0.0f));

    // dot3
    v0.set(1.0f, 0.0f, 0.0f, 0.0f);
    v1.set(1.0f, 0.0f, 0.0f, 0.0f);    
    this->Verify(float4::dot3(v0, v1) == 1.0f);
    v1.set(-1.0f, 0.0f, 0.0f, 0.0f);
    this->Verify(float4::dot3(v0, v1) == -1.0f);
    v1.set(0.0f, 1.0f, 0.0f, 0.0f);
    this->Verify(float4::dot3(v0, v1) == 0.0f);

    // @todo: test barycentric(), catmullrom(), hermite()

    // lerp
    v0.set(1.0f, 2.0f, 3.0f, 4.0f);
    v1.set(2.0f, 3.0f, 4.0f, 5.0f);
    v2 = float4::lerp(v0, v1, 0.5f);
    this->Verify(v2 == float4(1.5f, 2.5f, 3.5f, 4.5f));
    
    // maximize/minimize
    v0.set(1.0f, 2.0f, 3.0f, 4.0f);
    v1.set(4.0f, 3.0f, 2.0f, 1.0f);
    v2 = float4::maximize(v0, v1);
    this->Verify(v2 == float4(4.0f, 3.0f, 3.0f, 4.0f));
    v2 = float4::minimize(v0, v1);
    this->Verify(v2 == float4(1.0f, 2.0f, 2.0f, 1.0f));

    // normalize
    v0.set(2.5f, 0.0f, 0.0f, 0.0f);
    v1 = float4::normalize(v0);
    this->Verify(v1 == float4(1.0f, 0.0f, 0.0f, 0.0f));

    // transform (point and vector)
    matrix44 m = matrix44::translation(1.0f, 2.0f, 3.0f);
    v0.set(1.0f, 0.0f, 0.0f, 1.0f);
    v1 = float4::transform(v0, m);
    this->Verify(v1 == float4(2.0f, 2.0f, 3.0f, 1.0f));
    v0.set(1.0f, 0.0f, 0.0f, 0.0f);
    v1 = float4::transform(v0, m);
    this->Verify(v0 == float4(1.0f, 0.0f, 0.0f, 0.0f));

    // component-wise comparison
    v0.set(1.0f, 1.0f, 1.0f, 1.0f);
    v1.set(0.5f, 1.5f, 0.5f, 1.5f);
    v2.set(2.0f, 2.0f, 2.0f, 2.0f);
    this->Verify(float4::less4_any(v0, v1));
    this->Verify(float4::greater4_any(v0, v1));
    this->Verify(!float4::less4_all(v0, v1));
    this->Verify(!float4::greater4_all(v0, v1));
    this->Verify(float4::lessequal4_all(v0, v2));
    this->Verify(float4::greaterequal4_all(v2, v0));
}

}