#pragma once
//------------------------------------------------------------------------------
/**
    @file mathtestutil.h

    Comparison with Tolerance, Debug-Prints, Alignment-Macros

    (C) 2009 Radon Labs GmbH
*/

#include "core/debug.h"
#include "math/vec4.h"
#include "math/mat4.h"
#include "math/quat.h"
#include "math/plane.h"

using namespace Math;

//------------------------------------------------------------------------------
namespace Test
{

static const scalar TEST_EPSILON = 0.00001;
static const vec4 EPSILON4(TEST_EPSILON, TEST_EPSILON, TEST_EPSILON, TEST_EPSILON);
#define SCALAR_FORMATOR "%f"

//------------------------------------------------------------------------------
/**
    for different platforms, equal might need a different tolerance
    for results of computations
*/
__forceinline bool
vec4equal(const vec4 &a, const vec4 &b)
{
    return nearequal(a, b, TEST_EPSILON);
}

//------------------------------------------------------------------------------
/**
    for different platforms, equal might need a different tolerance
    for results of computations

    Note: 2 quaternions with the same numbers, but swapped signs represent the same rotation/orientation
*/
__forceinline bool
quaternionequal(const quat &a, const quat&b)
{
    return nearequal(vec4(a.x, a.y, a.z, a.w),
                              vec4(b.x, b.y, b.z, b.w),
                              EPSILON4)
               ||
          nearequal(vec4(-a.x, -a.y, -a.z, -a.w),
                              vec4(b.x, b.y, b.z, b.w),
                              EPSILON4);
}

//------------------------------------------------------------------------------
/**
    for different platforms, equal might need a different tolerance
    for results of computations
*/
__forceinline bool
mat4equal(const mat4 &a, const mat4 &b)
{
    return vec4equal(a.row0, b.row0) &&
           vec4equal(a.row1, b.row1) &&
           vec4equal(a.row2, b.row2) &&
           vec4equal(a.row3, b.row3);
}

//------------------------------------------------------------------------------
/**
    for different platforms, equal might need a different tolerance
    for results of computations
*/
__forceinline bool
scalarequal(scalar a, scalar b)
{
    return Math::nearequal(a, b, TEST_EPSILON);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
print(const vec4 &vec)
{
    n_printf( "( " SCALAR_FORMATOR " " SCALAR_FORMATOR " " SCALAR_FORMATOR " " SCALAR_FORMATOR " )\n", vec.x, vec.y, vec.z, vec.w );
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
print(const vec4 &vec, const char *msg)
{
    n_printf( "%s: ( " SCALAR_FORMATOR " " SCALAR_FORMATOR " " SCALAR_FORMATOR " " SCALAR_FORMATOR " )\n", msg, vec.x, vec.y, vec.z, vec.w );
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
print( const mat4& mat )
{
#if (__WIN32__ || __linux__)
    print(mat.row0);
    print(mat.row1);
    print(mat.row2);
    print(mat.row3);
#else
#  error unimplemented platform
#endif
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
print(const mat4 &mat, const char *msg)
{
    n_printf("%s:\n", msg);
    print( mat );
}

//------------------------------------------------------------------------------
/**
    print a matrix flattened, as it is linearily in memory
*/
__forceinline void
print_flattened(const mat4& mat, const char *msg)
{
    n_printf("%s:\n", msg);
    const float *flattened = (const float *)&(mat.row0);
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
print(const quat &q)
{
    if(scalarequal(length(q), 1.0))
    {
        const scalar angle = Math::acos(q.w) * 2.0f;
        n_printf("    axis     : " SCALAR_FORMATOR " " SCALAR_FORMATOR " " SCALAR_FORMATOR "\n"
                 "    w        : " SCALAR_FORMATOR "\n"
                 "    magnitude: " SCALAR_FORMATOR " rad: " SCALAR_FORMATOR " deg: " SCALAR_FORMATOR "\n",
                 q.x, q.y, q.z, q.w, length(q), angle, rad2deg(angle));
    }
    else
    {
        n_printf("    axis     : " SCALAR_FORMATOR " " SCALAR_FORMATOR " " SCALAR_FORMATOR "\n"
                 "    w        : " SCALAR_FORMATOR "\n"
                 "    magnitude: " SCALAR_FORMATOR "\n",
                 q.x, q.y, q.z, q.w, length(q));
    }
}

//------------------------------------------------------------------------------
/**
    print a quaternion
*/
__forceinline void
print(const quat&q, const char *msg)
{
    n_printf( "%s:\n", msg);
    print(q);
}

//------------------------------------------------------------------------------
/**
    for different platforms, equal might need a different tolerance
    for results of computations
*/
__forceinline bool
planeequal(const plane& a, const plane& b)
{
    return nearequal(vec4(a.a, a.b, a.c, a.d),
        vec4(b.a, b.b, b.c, b.d),
        EPSILON4);
}

} // namespace Test
