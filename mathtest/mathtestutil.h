#pragma once
//------------------------------------------------------------------------------
/**
    @file mathtestutil.h

    Comparison with Tolerance, Debug-Prints, Alignment-Macros

    (C) 2009 Radon Labs GmbH
*/

#include "core/debug.h"
#include "math/float4.h"
#include "math/matrix44.h"
#include "math/quaternion.h"
#include "math/plane.h"

using namespace Math;

//------------------------------------------------------------------------------
namespace Test
{

static const scalar TEST_EPSILON = 0.00001;
static const float4 EPSILON4(TEST_EPSILON, TEST_EPSILON, TEST_EPSILON, TEST_EPSILON);
#define SCALAR_FORMATOR "%f"

//------------------------------------------------------------------------------
/**
	for different platforms, equal might need a different tolerance
	for results of computations
*/
__forceinline bool
float4equal(const float4 &a, const float4 &b)
{
	return float4::nearequal4(a, b, EPSILON4);
}

//------------------------------------------------------------------------------
/**
	for different platforms, equal might need a different tolerance
	for results of computations

	Note: 2 quaternions with the same numbers, but swapped signs represent the same rotation/orientation
*/
__forceinline bool
quaternionequal(const quaternion &a, const quaternion &b)
{
	return float4::nearequal4(float4(a.x(), a.y(), a.z(), a.w()),
							  float4(b.x(), b.y(), b.z(), b.w()),
							  EPSILON4)
		       ||
		   float4::nearequal4(float4(-a.x(), -a.y(), -a.z(), -a.w()),
							  float4(b.x(), b.y(), b.z(), b.w()),
							  EPSILON4);
}

//------------------------------------------------------------------------------
/**
	for different platforms, equal might need a different tolerance
	for results of computations
*/
__forceinline bool
matrix44equal(const matrix44 &a, const matrix44 &b)
{
	return float4equal(a.getrow0(), b.getrow0()) &&
		   float4equal(a.getrow1(), b.getrow1()) &&
		   float4equal(a.getrow2(), b.getrow2()) &&
		   float4equal(a.getrow3(), b.getrow3());
}

//------------------------------------------------------------------------------
/**
	for different platforms, equal might need a different tolerance
	for results of computations
*/
__forceinline bool
scalarequal(scalar a, scalar b)
{
	return n_nearequal(a, b, TEST_EPSILON);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
print(const float4 &vec)
{
    n_printf( "( " SCALAR_FORMATOR " " SCALAR_FORMATOR " " SCALAR_FORMATOR " " SCALAR_FORMATOR " )\n", vec.x(), vec.y(), vec.z(), vec.w() );
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
print(const float4 &vec, const char *msg)
{
    n_printf( "%s: ( " SCALAR_FORMATOR " " SCALAR_FORMATOR " " SCALAR_FORMATOR " " SCALAR_FORMATOR " )\n", msg, vec.x(), vec.y(), vec.z(), vec.w() );
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
print( const matrix44& mat )
{
#if (__WIN32__ || __XBOX360__ || __WII__ || __linux__)
	print(mat.getrow0());
    print(mat.getrow1());
    print(mat.getrow2());
    print(mat.getrow3());
#elif __PS3__
/*
	inline const Vector4 Matrix4::getRow( int row ) const
	{
		return Vector4( mCol0.getElem( row ), mCol1.getElem( row ), mCol2.getElem( row ), mCol3.getElem( row ) );
	}
	inline const Vector4 Matrix4::getRow( int row ) const
	{
		return Vector4( mCol0.getElem( row ), mCol1.getElem( row ), mCol2.getElem( row ), mCol3.getElem( row ) );
	}
	inline void print( const Matrix4 & mat )
	{
		print( mat.getRow( 0 ) );
		print( mat.getRow( 1 ) );
		print( mat.getRow( 2 ) );
		print( mat.getRow( 3 ) );
	}

	our matrix44::getrow0 is actually a getcol, since this matrix is column-major-form, look at
	vectormath_aos.h declaration of Matrix4
*/
	// yes, looks weird, rows and cols seem to be mixed, look at the above matrix-funcs from mat_aos.h
    print(float4(mat.getrow0().x(), mat.getrow1().x(), mat.getrow2().x(), mat.getrow3().x()));
    print(float4(mat.getrow0().y(), mat.getrow1().y(), mat.getrow2().y(), mat.getrow3().y()));
    print(float4(mat.getrow0().z(), mat.getrow1().z(), mat.getrow2().z(), mat.getrow3().z()));
    print(float4(mat.getrow0().w(), mat.getrow1().w(), mat.getrow2().w(), mat.getrow3().w()));
#else
#  error unimplemented platform
#endif
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
print(const matrix44 &mat, const char *msg)
{
    n_printf("%s:\n", msg);
    print( mat );
}

//------------------------------------------------------------------------------
/**
	print a matrix flattened, as it is linearily in memory
*/
__forceinline void
print_flattened(const matrix44& mat, const char *msg)
{
	n_printf("%s:\n", msg);
	const float *flattened = (const float *)&(mat.getrow0());
	for(int i = 0; i < 16; ++i)
	{
		n_printf( "[%02d] " SCALAR_FORMATOR "\n", i, flattened[i] );
	}
}

//------------------------------------------------------------------------------
/**
	print a quaternion
*/
__forceinline void
print(const quaternion &q)
{
	if(scalarequal(q.length(), 1.0))
	{
		const scalar angle = n_acos(q.w()) * 2.0f;
		n_printf("    axis     : " SCALAR_FORMATOR " " SCALAR_FORMATOR " " SCALAR_FORMATOR "\n"
				 "    w        : " SCALAR_FORMATOR "\n"
				 "    magnitude: " SCALAR_FORMATOR " rad: " SCALAR_FORMATOR " deg: " SCALAR_FORMATOR "\n",
				 q.x(), q.y(), q.z(), q.w(), q.length(), angle, n_rad2deg(angle));
	}
    else
	{
		n_printf("    axis     : " SCALAR_FORMATOR " " SCALAR_FORMATOR " " SCALAR_FORMATOR "\n"
				 "    w        : " SCALAR_FORMATOR "\n"
				 "    magnitude: " SCALAR_FORMATOR "\n",
				 q.x(), q.y(), q.z(), q.w(), q.length());
	}
}

//------------------------------------------------------------------------------
/**
	print a quaternion
*/
__forceinline void
print(const quaternion &q, const char *msg)
{
	n_printf( "%s:\n", msg);
	print(q);
}

//------------------------------------------------------------------------------
/**
	print a plane
*/
__forceinline void
print(const plane &p)
{
	n_printf( "( " SCALAR_FORMATOR " " SCALAR_FORMATOR " " SCALAR_FORMATOR " " SCALAR_FORMATOR " )\n", p.a(), p.b(), p.c(), p.d() );
}

//------------------------------------------------------------------------------
/**
	print a plane
*/
__forceinline void
print(const plane &p, const char *msg)
{
	n_printf( "%s:\n", msg);
	print(p);
}

//------------------------------------------------------------------------------
/**
	for different platforms, equal might need a different tolerance
	for results of computations
*/
__forceinline bool
planeequal(const plane &a, const plane &b)
{
	return float4::nearequal4(float4(a.a(), a.b(), a.c(), a.d()),
							  float4(b.a(), b.b(), b.c(), b.d()),
							  EPSILON4);
}


}
