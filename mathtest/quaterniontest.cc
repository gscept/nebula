//------------------------------------------------------------------------------
//  quaterniontest.cc
//  (C) 2009 Radon Labs GmbH
//------------------------------------------------------------------------------
#include "stdneb.h"

#include "quaterniontest.h"
#include "math/quaternion.h"
#include "math/vector.h"
#include "mathtestutil.h"
#include "stackalignment.h"
#include "testbase/stackdebug.h"

using namespace Math;


namespace Test
{
__ImplementClass(Test::QuaternionTest, 'QTTS', Test::TestCase);

//------------------------------------------------------------------------------
/**
*/
void
QuaternionTest::Run()
{
    STACK_CHECKPOINT("Test::QuaternionTest::Run()");

	quaternion q0, q1, q2;

	// if numbers are the same, only all signeds are swapped, the quaternions represent the same rotation/orientation
	VERIFY(quaternionequal(quaternion(-0.525582f, -0.772620f, -0.133313f, -0.330225f), 
								 quaternion( 0.525582f,  0.772620f,  0.133313f,  0.330225f)));

	// inverse
	const quaternion qUnnormalized( 1.0f, 2.0f, -0.5, -2.0f);
	q0 = quaternion::inverse(qUnnormalized);
	VERIFY(quaternionequal(q0, quaternion(-0.108108f, -0.216216f, 0.054054f, -0.216216f)));

	// normalize
	// conjugated of a unit-q must be the same as its inverse
	const quaternion qNormalized = quaternion::normalize(qUnnormalized);
	VERIFY(scalarequal(1.0f, qNormalized.length()));
	VERIFY(scalarequal(1.0f, qNormalized.lengthsq()));
	q0 = quaternion::conjugate(qNormalized);
	q1 = quaternion::inverse(qNormalized);
	VERIFY(quaternionequal(q0, q1));

	// construction/assignment/comp�arison
	{
		quaternion a(1.0f, 2.0f, 3.0f, 4.0f);
		quaternion b(4.0f, 3.0f, 2.0f, 1.0f);
		quaternion aa(a);
		quaternion bb;
		bb = b;
		VERIFY(aa == a);
		VERIFY(bb == b);
		VERIFY(a != b);
		VERIFY(aa != bb);
		VERIFY(a == quaternion(1.0f, 2.0f, 3.0f, 4.0f));
	}

#ifndef __WII__
    // load and store aligned
    NEBULA_ALIGN16 const scalar fAlignedLoad[4] = { 1.0f, 2.0f, 3.0f, 4.0f };
    NEBULA_ALIGN16 scalar fAlignedStore[4];
	// check alignment
	n_assert(!((size_t)fAlignedLoad & 0xF));
	n_assert(!((size_t)fAlignedStore & 0xF));
    q0.load(fAlignedLoad);
    VERIFY(q0 == float4(1.0f, 2.0f, 3.0f, 4.0f));
	// load unaligned must work with aligned data too
    q0.loadu(fAlignedLoad);
    VERIFY(q0 == float4(1.0f, 2.0f, 3.0f, 4.0f));
    q0.store(fAlignedStore);
    VERIFY((fAlignedStore[0] == 1.0f) && (fAlignedStore[1] == 2.0f) && (fAlignedStore[2] == 3.0f) && (fAlignedStore[3] == 4.0f));
	// store unaligned must work with aligned data too
    q0.storeu(fAlignedStore);
    VERIFY((fAlignedStore[0] == 1.0f) && (fAlignedStore[1] == 2.0f) && (fAlignedStore[2] == 3.0f) && (fAlignedStore[3] == 4.0f));
    q0.stream(fAlignedStore);
    VERIFY((fAlignedStore[0] == 1.0f) && (fAlignedStore[1] == 2.0f) && (fAlignedStore[2] == 3.0f) && (fAlignedStore[3] == 4.0f));
	
	// load and store unaligned
	NEBULA_ALIGN16 const scalar fAlignedLoadBase[5] = { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f };
	NEBULA_ALIGN16 scalar fAlignedStoreBase[5];
	const scalar *fUnalignedLoad = fAlignedLoadBase + 1;
	scalar *fUnalignedStore = fAlignedStoreBase + 1;
	// check un-alignment
	n_assert(((size_t)fUnalignedLoad & 0xF));
	n_assert(((size_t)fUnalignedStore & 0xF));
    q0.loadu(fUnalignedLoad);
    VERIFY(q0 == float4(1.0f, 2.0f, 3.0f, 4.0f));
    q0.storeu(fUnalignedStore);
    VERIFY((fUnalignedStore[0] == 1.0f) && (fUnalignedStore[1] == 2.0f) && (fUnalignedStore[2] == 3.0f) && (fUnalignedStore[3] == 4.0f));
#endif

    // test 16-byte alignment of embedded members on the stack, if we use SSE/SSE2 on windows or
    // xbox or ps3
#if (__WIN32__ && !defined(_XM_NO_INTRINSICS_)) || __XBOX360__ || __PS3__    
    {
        testStackAlignment16<quaternion>(this);
    }
#endif

	// set
	q0.set(-1.0f, 3.0f, 2.05f, -222.463f);
	VERIFY(quaternionequal(q0, quaternion(-1.0f, 3.0f, 2.05f, -222.463f)));
	VERIFY(-1.0f == q0.x());
	VERIFY(3.0f == q0.y());
	VERIFY(2.05f == q0.z());
	VERIFY(-222.463f == q0.w());
	q0.x() = -99.0f;
	q0.y() = -100.0f;
	q0.z() = -101.0f;
	q0.w() = -102.0f;
	VERIFY(-99.0f == q0.x());
	VERIFY(-100.0f == q0.y());
	VERIFY(-101.0f == q0.z());
	VERIFY(-102.0f == q0.w());
	q0.set_x(-199.0f);
	q0.set_y(-1100.0f);
	q0.set_z(-1101.0f);
	q0.set_w(-1102.0f);
	VERIFY(-199.0f == q0.x());
	VERIFY(-1100.0f == q0.y());
	VERIFY(-1101.0f == q0.z());
	VERIFY(-1102.0f == q0.w());

	// isidentity
	q0.set(0.0f, 0.0f, 0.0f, 1.0f);
	VERIFY(q0.isidentity());
	// length lengthsq
	VERIFY(scalarequal(1.0f, q0.length()));
	VERIFY(scalarequal(1.0f, q0.lengthsq()));
	q0.set(2.0f, 3.0f, 4.0f, 5.0f);
	VERIFY(scalarequal(7.348469f, q0.length()));
	VERIFY(scalarequal(54.0f, q0.lengthsq()));
	// conjugate
	q1 = quaternion::conjugate(q0);
	VERIFY(quaternionequal(q1, quaternion(-2.0f,-3.0f,-4.0f,5.0f)));

	// dot
	q0 = quaternion::normalize(quaternion(2.0f, 3.0f, 4.0f, 5.0f));
	q1 = quaternion::normalize(quaternion(-1.0f, -223.0f, 0.0f, 9.0f));
	VERIFY(scalarequal(-0.381694f, quaternion::dot(q0, q1)));

	// identity
	q0 = quaternion::identity();
	VERIFY(q0.isidentity());
	VERIFY(q0 == quaternion(0.0f, 0.0f, 0.0f, 1.0f));

	// multiply
	q0 = quaternion::normalize(quaternion(2.0f, 3.0f, 4.0f, 5.0f));
	q1 = quaternion::normalize(quaternion(-1.0f, -223.0f, 0.0f, 9.0f));
	q2 = quaternion::multiply(q0, q1);
	VERIFY(quaternionequal(q2, quaternion(-0.535956f, -0.660952f, 0.292063f, 0.436570f)));

	// rotationaxis
	const vector rotaxis = float4::normalize(vector(1.0f, 3.0f, 5.0f));
	q0 = quaternion::rotationaxis(rotaxis, -1.348f);
	VERIFY(quaternionequal(q0, quaternion(-0.105495f, -0.316485f, -0.527475f, 0.781331f)));

	// rotationmatrix
	matrix44 rot = matrix44::multiply(matrix44::rotationx(1.0f), matrix44::rotationy(-2.0f));
	q0 = quaternion::rotationmatrix(rot);
	VERIFY(quaternionequal(q0, quaternion(-0.259035f, 0.738460f, -0.403423f, -0.474160f)));

	q0 = matrix44::rotationmatrix(rot);
	VERIFY(quaternionequal(q0, quaternion(-0.259035f, 0.738460f, -0.403423f, -0.474160f)));

	// rotationyawpitchroll
	q0 = quaternion::rotationyawpitchroll(-7.0f, 3.0f, -2.0f);
	VERIFY(quaternionequal(q0, quaternion(0.525582f, 0.772620f, 0.133313f, 0.330225f)));

	// 2 quaternions with the same numbers, but swapped signs represent the same rotation/orientation,
	// thus the result-matrix must be the same
	const matrix44 ma = matrix44::rotationquaternion(quaternion( 0.525582f, -0.772620f,  0.133313f,  0.330225f));
	const matrix44 mb = matrix44::rotationquaternion(quaternion(-0.525582f,  0.772620f, -0.133313f, -0.330225f));
	VERIFY(matrix44equal(ma, mb));

	// slerp
	q0 = quaternion::normalize(quaternion(1.0f, 2.0f, 3.0f, 4.0f));
	q1 = quaternion::normalize(quaternion(4.0f, 3.0f, 2.0f, 1.0f));
	q2 = quaternion::slerp(q0, q1, 0.0f);
	VERIFY(quaternionequal(q0, q2));
	q2 = quaternion::slerp(q0, q1, 1.0f);
	VERIFY(quaternionequal(q1, q2));
	q2 = quaternion::slerp(q0, q1, -0.15f);
	VERIFY(quaternionequal(q2, quaternion(0.078387f, 0.310878f, 0.543369f, 0.775861f)));
	q2 = quaternion::slerp(q0, q1, 0.15f);
	VERIFY(quaternionequal(q2, quaternion(0.283860f, 0.413614f, 0.543369f, 0.673124f)));
	q2 = quaternion::slerp(q0, q1, 0.5f);
	VERIFY(quaternionequal(q2, quaternion(0.500000f, 0.500000f, 0.500000f, 0.500000f)));
	q2 = quaternion::slerp(q0, q1, 0.789f);
	VERIFY(quaternionequal(q2, quaternion(0.646757f, 0.539120f, 0.431484f, 0.323847f)));
	q2 = quaternion::slerp(q0, q1, 1.1f);
	VERIFY(quaternionequal(q2, quaternion(0.762011f, 0.545786f, 0.329562f, 0.113337f)));

	// to_axisangle
	q0 = quaternion::normalize(quaternion(1.0f, 2.0f, 3.0f, 4.0f));
	float4 outAxis;
	scalar outAngle;
	quaternion::to_axisangle(q0, outAxis, outAngle);
	VERIFY(float4equal(outAxis, float4(0.182574f, 0.365148f, 0.547723f, 0.0f)));
	VERIFY(scalarequal(outAngle, 1.504080f));

	// exp
	q0 = quaternion::normalize(quaternion(1.0f, 2.0f, 3.0f, 4.0f));
	q0 = quaternion::exp(q0);
	VERIFY(quaternionequal(q0, quaternion(0.168702f, 0.337403f, 0.506105f, 0.775601f)));

	// ln
	q0 = quaternion::normalize(quaternion(1.0f, 2.0f, 3.0f, 4.0f));
	q0 = quaternion::ln(q0);
	VERIFY(quaternionequal(q0, quaternion(0.200991f, 0.401982f, 0.602974f, 0.0f)));

	// todo barycentric
}

}