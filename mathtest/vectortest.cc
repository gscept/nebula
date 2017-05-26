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

	// construction
    vector t;
    this->Verify(float4equal(t, float4(0.0, 0.0, 0.0, 0.0)));
    vector z(t);
    this->Verify(float4equal(z, float4(0.0, 0.0, 0.0, 0.0)));
	vector v(1.0, 2.0, 3.0);
	this->Verify(float4equal(v, float4(1.0, 2.0, 3.0, 0.0)));
    vector v0(1.0f, 2.0f, 3.0f);
    vector v1(4.0f, 3.0f, 2.0f);
    vector v2(v0);
    vector v3(v1);
    this->Verify(v0 == v2);
    this->Verify(v1 == v3);
    this->Verify(v0 != v1);
    this->Verify(v2 != v3);
    this->Verify(v0 == float4(1.0f, 2.0f, 3.0f, 0.0));
    // assignemt
    v2 = v1;
    this->Verify(v2 == v1);
    v2 = v0;
    this->Verify(v2 == v0);

    // test 16-byte alignment of embedded members on the stack, if we use SSE/SSE2 on windows or
    // xbox or ps3
#if (__WIN32__ && !defined(_XM_NO_INTRINSICS_)) || __XBOX360__ || __PS3__    
    {
        testStackAlignment16<vector>(this);
    }
#endif
}

}