#pragma once
//------------------------------------------------------------------------------
/**
    @file math/vecmath/vec_scalar.h
    
    Scalar typedef and math functions for VectorMath math functions.
        
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

namespace Math
{
typedef float scalar;

const scalar LN_2 = 0.693147180559945f;

#ifndef PI
#define PI (3.1415926535897932384626433832795028841971693993751)
#endif
// the half circle
#ifndef N_PI
#define N_PI (Math::scalar(3.1415926535897932384626433832795028841971693993751))
#endif

#define _DECLSPEC_ALIGN_16_ __declspec(align(16))
//------------------------------------------------------------------------------
/**
    Return a pseudo random number between 0 and 1.
*/
__forceinline scalar 
n_rand()
{
    return scalar(rand()) / scalar(RAND_MAX);
}

//------------------------------------------------------------------------------
/**
    Return a pseudo random number between min and max.
*/
__forceinline scalar 
n_rand(scalar min, scalar max)
{
	scalar unit = scalar(rand()) / RAND_MAX;
	scalar diff = max - min;
	return min + unit * diff;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_sin(scalar x)
{
    return sinf(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_cos(scalar x)
{
    return cosf(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_tan(scalar x)
{
    return tanf(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_asin(scalar x)
{
    return asinf(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_acos(scalar x)
{
    return acosf(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_atan(scalar x)
{
    return atanf(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar 
n_sqrt(scalar x)
{
    return sqrtf(x);
}

//------------------------------------------------------------------------------
/**
    Chop float to int.
*/
__forceinline int 
n_fchop(scalar f)
{
    /// @todo type cast to int is slow!
    return int(f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_fmod(scalar x, scalar y)
{
	return fmodf(x, y);
}

//------------------------------------------------------------------------------
/**
    Normalize an angular value into the range rad(0) to rad(360).
	FIXME : seems that the xna version limits from -pi to pi, not 0 .. 2pi. 
			will copy behaviour despite what description says
*/
__forceinline scalar 
n_modangle(scalar a) 
{
#if 0
	static const scalar rev = scalar(6.283185307179586476925286766559);
	return n_fmod(a,rev);
#else
	static const scalar REVOLUTION = scalar(6.283185307179586476925286766559);
	a += N_PI;
	scalar temp = fabs(a);
	temp = temp - floorf(temp/REVOLUTION) *  REVOLUTION;
	temp -= N_PI;
	temp = a<0 ? -temp:temp;	
	if(temp < scalar(-N_PI)) temp += REVOLUTION;
	if(temp >= scalar(N_PI)) temp -= REVOLUTION;
	return temp;
#endif
}

//------------------------------------------------------------------------------
/**
    log2() function.
*/
__forceinline scalar 
n_log2(scalar f) 
{ 
    return logf(f) / LN_2; 
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_exp(scalar x)
{
    return expf(x);
}

//------------------------------------------------------------------------------
/**
    Round float to integer.
*/
__forceinline int 
n_frnd(scalar f)
{
    return n_fchop(floorf(f + 0.5f));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_pow(scalar x, scalar y)
{
    return powf(x, y);
}

//------------------------------------------------------------------------------
/**
	get logarithm of x
*/
__forceinline scalar
n_log(scalar x)
{
    return logf(x);
}

} // namespace Math
//------------------------------------------------------------------------------



    
