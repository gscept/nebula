#pragma once
//------------------------------------------------------------------------------
/**
    SSE support header

    (C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"

#define NEBULA_USE_SSE_FMA 0

namespace Math
{

//------------------------------------------------------------------------------
/**
    Fused multiply-add operation, (a * b) + c
*/
__forceinline __m128 
fmadd(__m128 a, __m128 b, __m128 c)
{
#if NEBULA_USE_SSE_FMA    
    return _mm_fmadd_ps(a, b, c);
#else
    return _mm_add_ps(_mm_mul_ps(a, b), c);
#endif
}


//------------------------------------------------------------------------------
/**
    Constructs a vector of results where each element corresponds to a[i] < b[i] with either 0 or 1
*/
__forceinline __m128
less(__m128 a, __m128 b)
{
    return _mm_min_ps(_mm_cmplt_ps(a, b), _mm_set1_ps(1));
}

//------------------------------------------------------------------------------
/**
    Constructs a vector of results where each element corresponds to a[i] > b[i] with either 0 or 1
*/
__forceinline __m128
greater(__m128 a, __m128 b)
{
    return _mm_min_ps(_mm_cmpeq_ps(a, b), _mm_set1_ps(1));
}

} // namespace Math
