//------------------------------------------------------------------------------
//  pointtest.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "pointtest.h"
#include "math/point.h"
#include "mathtestutil.h"
#include "stackalignment.h"
#include "testbase/stackdebug.h"

using namespace Math;


namespace Test
{
__ImplementClass(Test::PointTest, 'PTTS', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
PointTest::Run()
{
    STACK_CHECKPOINT("Test::PointTest::Run()");

	// construction
    point t;
    this->Verify(float4equal(t, float4(0.0, 0.0, 0.0, 1.0)));
    point z(t);
    this->Verify(float4equal(z, float4(0.0, 0.0, 0.0, 1.0)));
	point p(1.0, 2.0, 3.0);
	this->Verify(float4equal(p, float4(1.0, 2.0, 3.0, 1.0)));
    point v0(1.0f, 2.0f, 3.0f);
    point v1(4.0f, 3.0f, 2.0f);
    point v2(v0);
    point v3(v1);
    this->Verify(v0 == v2);
    this->Verify(v1 == v3);
    this->Verify(v0 != v1);
    this->Verify(v2 != v3);
    this->Verify(v0 == float4(1.0f, 2.0f, 3.0f, 1.0));
    // assignemt
    v2 = v1;
    this->Verify(v2 == v1);
    v2 = v0;
    this->Verify(v2 == v0);

    // test 16-byte alignment of embedded members on the stack, if we use SSE/SSE2 on windows or
    // xbox or ps3
#if (__WIN32__ && !defined(_XM_NO_INTRINSICS_)) || __XBOX360__ || __PS3__    
    {
        testStackAlignment16<point>(this);
    }
#endif
}

}