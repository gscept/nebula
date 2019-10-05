#pragma once
//------------------------------------------------------------------------------
/**
    @file math/scalar.h

    Nebula's scalar datatype.

    NOTE: do not add CRT math function calls to this call, but instead
    into the platform specific headers (for instance, on the Wii the sinf()
    functions are called and must be placed into a .cc file, not into the
    header.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/

#if __USE_MATH_DIRECTX
#include "math/xnamath/xna_scalar.h"
#elif __USE_VECMATH
#include "math/vecmath/vec_scalar.h"
#else
#error "scalar class not implemented!"
#endif

// common platform-agnostic definitions
namespace Math
{
#ifndef PI
#define PI (3.1415926535897932384626433832795028841971693993751)
#endif

// the half circle
#ifndef N_PI
#define N_PI (Math::scalar(PI))
#endif
// the whole circle
#ifndef N_PI_DOUBLE
#define N_PI_DOUBLE (Math::scalar(6.283185307179586476925286766559))
#endif
// a quarter circle
#ifndef N_PI_HALF
#define N_PI_HALF (Math::scalar(1.5707963267948966192313216916398f))
#endif
// ---HOTFIXED

#ifndef TINY
#define TINY (0.0000001f)
#endif
#define N_TINY TINY

//------------------------------------------------------------------------------
/**
    A fuzzy floating point equality check
*/
__forceinline bool
n_fequal(scalar f0, scalar f1, scalar tol)
{
    scalar f = f0 - f1;
    return ((f > (-tol)) && (f < tol));
}

#if !SPU

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_max(scalar a, scalar b)
{
    return (a > b) ? a : b;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline double
n_max(double a, double b)
{
    return (a > b) ? a : b;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int
n_max(int a, int b)
{
    return (a > b) ? a : b;
}

//------------------------------------------------------------------------------
/**
    branchless max for uint32
*/
__forceinline unsigned int
n_max(unsigned int a, unsigned int b)
{
    return a ^ ((a ^ b) & -(a < b));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_min(scalar a, scalar b)
{
    return (a < b) ? a : b;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline double
n_min(double a, double b)
{
    return (a < b) ? a : b;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int
n_min(int a, int b)
{
    return (a < b) ? a : b;
}

//------------------------------------------------------------------------------
/**
    branchless min for uints
*/
__forceinline unsigned int
n_min(unsigned int a, unsigned int b)
{
    return b ^ ((a ^ b) & -(a < b));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_abs(scalar a)
{
    return (a < 0.0f) ? -a : a;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline double
n_abs(double a)
{
    return (a < 0.0f) ? -a : a;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline int
n_abs(int a)
{
    return (a < 0.0f) ? -a : a;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_sgn(scalar a)
{
    return (a < 0.0f) ? -1.0f : 1.0f;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline constexpr scalar
n_deg2rad(scalar d)
{
    return (scalar)((d * PI) / 180.0f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline constexpr scalar
n_rad2deg(scalar r)
{
    return (scalar)((r * 180.0f) / PI);
}

//------------------------------------------------------------------------------
/**
    Integer clamping.
*/
__forceinline int
n_iclamp(int val, int minVal, int maxVal)
{
    if (val < minVal)      return minVal;
    else if (val > maxVal) return maxVal;
    else return val;
}

//------------------------------------------------------------------------------
/**
	Floating point ceiling
*/
__forceinline float
n_ceil(float val)
{
	return ceilf(val);
}

//------------------------------------------------------------------------------
/**
	Floating point flooring
*/
__forceinline float
n_floor(float val)
{
	return floorf(val);
}

//------------------------------------------------------------------------------
/**
    A fuzzy floating point less-then check.
*/
__forceinline bool
n_fless(scalar f0, scalar f1, scalar tol)
{
    return ((f0 - f1) < tol);
}

//------------------------------------------------------------------------------
/**
    A fuzzy floating point greater-then check.
*/
__forceinline bool
n_fgreater(scalar f0, scalar f1, scalar tol)
{
    return ((f0 - f1) > tol);
}

//------------------------------------------------------------------------------
/**
    Clamp a value against lower und upper boundary.
*/
__forceinline scalar
n_clamp(scalar val, scalar lower, scalar upper)
{
    if (val < lower)      return lower;
    else if (val > upper) return upper;
    else                  return val;
}

//------------------------------------------------------------------------------
/**
    Saturate a value (clamps between 0.0f and 1.0f)
*/
__forceinline scalar
n_saturate(scalar val)
{
    if (val < 0.0f)      return 0.0f;
    else if (val > 1.0f) return 1.0f;
    else return val;
}

//------------------------------------------------------------------------------
/**
Saturate a value (clamps between 0.0f and 1.0f)
*/
__forceinline double
n_saturate(double val)
{
    if (val < 0.0)      return 0.0;
    else if (val > 1.0) return 1.0;
    else return val;
}

//------------------------------------------------------------------------------
/**
    Linearly interpolate between 2 values: ret = x + l * (y - x)
*/
__forceinline scalar
n_lerp(scalar x, scalar y, scalar l)
{
    return x + l * (y - x);
}

//------------------------------------------------------------------------------
/**
    Linearly interpolate between 2 values: ret = x + l * (y - x)
*/
__forceinline double
n_lerp(double x, double y, double l)
{
    return x + l * (y - x);
}

//------------------------------------------------------------------------------
/**
    Get angular distance.
*/
__forceinline scalar
n_angulardistance(scalar from, scalar to)
{
	scalar normFrom = n_modangle(from);
    scalar normTo   = n_modangle(to);
    scalar dist = normTo - normFrom;
    if (dist < n_deg2rad(-180.0f))
    {
        dist += n_deg2rad(360.0f);
    }
    else if (dist > n_deg2rad(180.0f))
    {
        dist -= n_deg2rad(360.0f);
    }
    return dist;
}

//------------------------------------------------------------------------------
/**
    Returns true if the input scalar is denormalized (#DEN)
*/
__forceinline bool
n_isdenormal(scalar s)
{
#if __GNUC__
    union { scalar s; uint u; } pun;
    pun.s = s;
    return ((pun.u&0x7f800000)==0);
#else
    return (((*(uint*)&s)&0x7f800000)==0);
#endif
}

//------------------------------------------------------------------------------
/**
    Returns 0 if scalar is denormal.
*/
__forceinline float
n_undenormalize(scalar s)
{
    if (n_isdenormal(s))
    {
        return 0.0f;
    }
    else
    {
        return s;
    }
}

//------------------------------------------------------------------------------
/**
    test of nearly equal given a tolerance (epsilon)
*/
__forceinline bool
n_nearequal(scalar a, scalar b, scalar epsilon)
{
    return n_abs(a - b) <= epsilon;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_cot(scalar x)
{
    return scalar(1.0) / n_tan(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_smoothstep(scalar edge0, scalar edge1, scalar x)
{
    // Scale, bias and saturate x to 0..1 range
    x = n_saturate((x - edge0) / (edge1 - edge0)); 
    // Evaluate polynomial
    return x*x*(3-2*x);     
}

//------------------------------------------------------------------------------
/**
    Return a pseudo integer random number between min and max.
*/
__forceinline int 
n_irand(int min, int max)
{	
	int range = max - min + 1;
	int unit = rand() % range;
	return min + unit;
}

//------------------------------------------------------------------------------
/**
	Returns the position of the most significant bit of the number
*/
__forceinline int
n_mostsignificant(uint val)
{
#ifdef WIN32
	unsigned long ret;
	n_assert2(_BitScanReverse(&ret, val),"failed to calculate most significant bit\n");
	return ret + 1;
#else
	n_error("not implemented\n");
#endif
}

//------------------------------------------------------------------------------
/**
*/
__forceinline uint
n_align(uint alignant, uint alignment)
{
	return (alignant + alignment - 1) & ~(alignment - 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline uint
n_divandroundup(uint dividend, uint divider)
{
	return (dividend % divider != 0) ? (dividend / divider + 1) : (dividend / divider);
}

#endif // #if !SPU

} // namespace Math
//------------------------------------------------------------------------------
