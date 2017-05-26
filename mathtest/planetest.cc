//------------------------------------------------------------------------------
//  planetest.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "planetest.h"
#include "math/plane.h"
#include "math/point.h"
#include "mathtestutil.h"
#include "stackalignment.h"
#include "testbase/stackdebug.h"

using namespace Math;

namespace Test
{
__ImplementClass(Test::PlaneTest, 'PLTS', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
PlaneTest::Run()
{
    STACK_CHECKPOINT("Test::PlaneTest::Run()");

	/// construct from components
	{		
		const plane p(1.0f, 2.0f, 3.0f, 4.0f);
		this->Verify(planeequal(p, plane(1.0f, 2.0f, 3.0f, 4.0f)));
	}
	/// construct from points
	{		
		const point triangle[3] = {point(1.0f,0.0f,1.0f), point(-5.0f,-2.0f,-1.0f), point(0.0f,0.0f,-1.0f)};
	    const plane p(triangle[0], triangle[1], triangle[2]);
		this->Verify(planeequal(p, plane(0.365148f, -0.912871f, -0.182574f, -0.182574f)));
	}
    /// construct from point and normal
	{
		const point p(1.0f,0.0f,1.0f);
		const vector n = float4::normalize(float4(1.0f,-7.0f,2.0f,0.0f));
		const plane pl(p, n);
		this->Verify(planeequal(pl, plane(0.136083f, -0.952579f, 0.272166f, -0.408248f)));
		/// copy constructor
		const plane copy(pl);
		this->Verify(planeequal(pl, copy));
	}
	// set
	plane pl;
	pl.set(0.136083f, -0.952579f, 0.272166f, -0.408248f);
	this->Verify(planeequal(pl, plane(0.136083f, -0.952579f, 0.272166f, -0.408248f)));
	// accessors ..
	this->Verify(scalarequal(pl.a(),  0.136083f));
	this->Verify(scalarequal(pl.b(), -0.952579f));
	this->Verify(scalarequal(pl.c(),  0.272166f));
	this->Verify(scalarequal(pl.d(), -0.408248f));
	pl.a() = -7.0f;
	this->Verify(scalarequal(pl.a(), -7.0f));
	pl.b() = 23.2378296f;
	this->Verify(scalarequal(pl.b(), 23.2378296f));
	pl.c() = 1293.4f;
	this->Verify(scalarequal(pl.c(), 1293.4f));
	pl.d() = -65.89f;
	this->Verify(scalarequal(pl.d(), -65.89f));
	pl.set_a(-3.0f);
	this->Verify(scalarequal(pl.a(), -3.0f));
	pl.set_b(55.2378296f);
	this->Verify(scalarequal(pl.b(), 55.2378296f));
	pl.set_c(51293.4f);
	this->Verify(scalarequal(pl.c(), 51293.4f));
	pl.set_d(5.89f);
	this->Verify(scalarequal(pl.d(), 5.89f));

	// dot
	pl.set(0.136083f, -0.952579f, 0.272166f, -0.408248f);
	scalar s = pl.dot(float4(3.0f, -23.0f, 33.0f, 1.0f));
	this->Verify(scalarequal(30.890797f, s));
	// intersectline
	{
		pl.set(0.0f, 1.0f, 0.0f, 0.0f);
		point startPoint(-2.0f, -2.0f, 1.0f);
		point endPoint(  -2.0f,  5.0f, 1.0f);
		point interPoint;
		bool intersection = pl.intersectline(startPoint, endPoint, interPoint);
		this->Verify(intersection);
		this->Verify(float4equal(interPoint, float4(-2.0f, 0.0f, 1.0f, 1.0f)));
		pl.set(0.0f, 1.0f, 0.0f, 0.0f);
		startPoint.set(-2.0f,  1.0f, 1.0f);
		endPoint.set(  -2.0f,  5.0f, 1.0f);
		intersection = pl.intersectline(startPoint, endPoint, interPoint);
		this->Verify(intersection);
		this->Verify(float4equal(interPoint, float4(-2.0f, 0.0f, 1.0f, 1.0f)));
		// line parallel to plane, no intersection
		startPoint.set(1.0f,  1.0f, 1.0f);
		endPoint.set(  2.0f,  1.0f, 1.0f);
		intersection = pl.intersectline(startPoint, endPoint, interPoint);
		this->Verify(!intersection);
	}
	// normalize
	pl.set(17.0f, -21.0f, 23.3425f, 13.2434f);
	pl = plane::normalize(pl);
	this->Verify(planeequal(pl, plane(0.476119f, -0.588147f, 0.653754f, 0.370908f)));
	// transform
	pl.set(17.0f, -21.0f, 23.3425f, 13.2434f);
	pl = plane::normalize(pl);
	const matrix44 mRotOneX_Trans123 = matrix44::multiply(matrix44::rotationx(1.0f), matrix44::translation(1.0f, 2.0f, 3.0f));
	pl = plane::transform(pl, mRotOneX_Trans123);
	this->Verify(planeequal(pl, plane(0.847027f, -0.126076f, 0.971040f, 0.370908f)));

    // test 16-byte alignment of embedded members on the stack, if we use SSE/SSE2 on windows or
    // xbox or ps3
#if (__WIN32__ && !defined(_XM_NO_INTRINSICS_)) || __XBOX360__ || __PS3__    
    {
        testStackAlignment16<plane>(this);
    }
#endif
}

}