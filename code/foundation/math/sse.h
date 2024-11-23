#pragma once
//------------------------------------------------------------------------------
/**
    @file sse.h
    
    SSE support functions

    @copyright
    (C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#ifdef __WIN32__ || __linux__
#include <xmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>
#elif __APPLE__
#include <sse2neon.h>
#endif

namespace Math
{

//------------------------------------------------------------------------------
/**
    Fused multiply-add operation, (a * b) + c
*/
__forceinline __m128 
fmadd(__m128 a, __m128 b, __m128 c)
{
#if N_USE_FMA    
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
    return _mm_min_ps(_mm_cmpgt_ps(a, b), _mm_set1_ps(1));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline __m128
recip(__m128 a)
{
    return _mm_div_ps(_mm_set1_ps(1), a);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline __m128
mul(__m128 a, __m128 b)
{
    return _mm_mul_ps(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline __m128
div(__m128 a, __m128 b)
{
    return _mm_div_ps(a, b);
}

//------------------------------------------------------------------------------
/**
*/
template<int x, int y, int z, int w>
__forceinline __m128
swizzle(__m128 v)
{
    return _mm_shuffle_ps(v, v, _MM_SHUFFLE(x, y, z, w));
}

} // namespace Math
