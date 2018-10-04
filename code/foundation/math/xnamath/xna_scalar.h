#pragma once
//------------------------------------------------------------------------------
/**
    @file math/xbox360/scalar.h
    
    Scalar typedef and math functions for Xbox360 math functions.
    
    (C) 2007 Radon Labs GmbH
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
#define N_PI (Math::scalar(PI))
#endif

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
    return DirectX::XMScalarSin(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_cos(scalar x)
{
    return DirectX::XMScalarCos(x);
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
    return DirectX::XMScalarASin(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_acos(scalar x)
{
    return DirectX::XMScalarACos(x);
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
#if __XBOX360__
    return __fsqrts(x);
#else
    return sqrtf(x);
#endif
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
    Normalize an angular value into the range rad(0) to rad(360).
*/
__forceinline scalar 
n_modangle(scalar a) 
{
    // we had different results for
    //   win32 release:  3.141593 == XMScalarModAngle(N_PI)
    //   win32 debug  : -3.141593 == XMScalarModAngle(N_PI)
    // so we have to map it on our own 
    static const scalar REVOLUTION = scalar(6.283185307179586476925286766559);
    scalar ret = DirectX::XMScalarModAngle(a);
    if(ret < scalar(-N_PI)) ret += REVOLUTION;
    if(ret >= scalar(N_PI)) ret -= REVOLUTION;
    return ret;
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
n_fmod(scalar x, scalar y)
{
    return fmodf(x, y);
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



    