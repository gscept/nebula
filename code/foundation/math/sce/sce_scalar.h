#pragma once
#ifndef MATH_SCE_SCALAR_H
#define MATH_SCE_SCALAR_H
//------------------------------------------------------------------------------
/**
    @file math/sce/sce_scalar.h
    
    Scalar typedef and math functions for SCE math functions.
    
    (C) 2007 Radon Labs GmbH
*/
#include "core/types.h"

namespace Math
{
typedef float scalar;

#ifndef PI
#define PI (3.1415926535897932384626433832795028841971693993751f)
#endif
#define N_PI PI

#ifndef TINY
#define TINY (0.0000001f)
#endif
#define N_TINY TINY

const scalar LN_2 = 0.693147180559945f;

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
n_tan(scalar x)
{
    return tanf(x);
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
*/
__forceinline scalar
n_exp(scalar x)
{
    return expf(x);
}


//------------------------------------------------------------------------------
/**
    Smooth a new value towards an old value using a change value.
*/
__forceinline scalar
n_smooth(scalar newVal, scalar curVal, scalar maxChange)
{
    scalar diff = newVal - curVal;
    if (fabs(diff) > maxChange)
    {
        if (diff > 0.0f)
        {
            curVal += maxChange;
            if (curVal > newVal)
            {
                curVal = newVal;
            }
        }
        else if (diff < 0.0f)
        {
            curVal -= maxChange;
            if (curVal < newVal)
            {
                curVal = newVal;
            }
        }
    }
    else
    {
        curVal = newVal;
    }
    return curVal;
}

//------------------------------------------------------------------------------
/**
    Return a pseudo random number between 0 and 1.
*/
__forceinline scalar n_rand()
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
    Normalize an angular value into the range rad(0) to rad(360).
*/
__forceinline scalar 
n_modangle(scalar a) 
{
    // FIXME: hmm...
    static const scalar REVOLUTION = scalar(6.283185307179586476925286766559);
    while(a < 0.0f)
    {
        a += REVOLUTION;
    }
    if (a >= REVOLUTION)
    {
        a = n_fmod(a, REVOLUTION);
    }
    return a;
}



//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
n_pow(scalar x, scalar y)
{
    return powf(x, y);
}

} // namespace Math
//------------------------------------------------------------------------------
#endif

