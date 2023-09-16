//------------------------------------------------------------------------------
//  vectortest.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "vectortest.h"
#include "math/vector.h"
#include "mathtestutil.h"
#include "stackalignment.h"
#include "testbase/stackdebug.h"

using namespace Math;


namespace Test
{
__ImplementClass(Test::VectorTest, 'VCTS', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
VectorTest::Run()
{
    STACK_CHECKPOINT("Test::VectorTest::Run()");

    __m128 m128point = _mm_setr_ps(1, 2, 3, 1);

    // construction
    vector t;
    VERIFY(vec4equal(t, vec4(0.0, 0.0, 0.0, 0.0)));
    vector z(t);
    VERIFY(vec4equal(z, vec4(0.0, 0.0, 0.0, 0.0)));
    vector m(m128point);
    VERIFY(vec4equal(m, vec4(1.0f, 2.0f, 3.0f, 0.0f)));
    vector v(1.0, 2.0, 3.0);
    VERIFY(vec4equal(v, vec4(1.0, 2.0, 3.0, 0.0)));
    vector v0(1.0f, 2.0f, 3.0f);
    vector v1(4.0f, 3.0f, 2.0f);
    vector v2(v0);
    vector v3(v1);
    VERIFY(v0 == v2);
    VERIFY(v1 == v3);
    VERIFY(v0 != v1);
    VERIFY(v2 != v3);
    VERIFY(vec4equal(v0, vec4(1.0f, 2.0f, 3.0f, 0.0f)));
    // assignemt
    v2 = v1;
    VERIFY(v2 == v1);
    v2 = v0;
    VERIFY(v2 == v0);
    vector m2;
    m2 = m128point;
    VERIFY(m == m2);

    // test 16-byte alignment of embedded members on the stack, if we use SSE/SSE2 on windows or
    // xbox or ps3
#if (__WIN32__ && !defined(_XM_NO_INTRINSICS_))
    {
        testStackAlignment16<vector>(this);
    }
#endif
}

}