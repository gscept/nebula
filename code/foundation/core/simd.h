#pragma once
//------------------------------------------------------------------------------
/**
    @file core/simd.h
    
    Maps generic SIMD-like types and intrinsics to either SSE4+AVX or NEON
    
    (C) 2025 Individual contributors, see AUTHORS file
*/

#if NEBULA_SIMD_X64
#include <xmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>
typedef __m128 f32x4;
typedef __m128i i32x4;
typedef __m128i u32x4;

f32x4 cast_i32x4_to_f32x4(i32x4 x);
i32x4 set_i32x4(int32_t x, int32_t y, int32_t z, int32_t w);
static const f32x4 _mask_xyz = cast_i32x4_to_f32x4(set_i32x4( 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0 ));

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
set_f32x4(float x, float y, float z, float w)
{
    return _mm_setr_ps(x,y,z,w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline i32x4
set_i32x4(int32_t x, int32_t y, int32_t z, int32_t w)
{
    return _mm_setr_epi32(x,y,z,w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
splat_f32x4(float x)
{
    return _mm_set1_ps(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
cast_i32x4_to_f32x4(i32x4 x)
{
    return _mm_castsi128_ps(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
set_last_f32x4(f32x4 v, float val)
{
    f32x4 vec = _mm_set_ss(val);
    return _mm_insert_ps(v, vec, 0b00110000);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline u32x4
compare_equal_f32x4(f32x4 a, f32x4 b)
{
    return _mm_castps_si128(_mm_cmpeq_ps(a, b));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline u32x4
compare_greater_equal_f32x4(f32x4 a, f32x4 b)
{
    return _mm_castps_si128(_mm_cmpge_ps(a, b));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline u32x4
compare_greater_f32x4(f32x4 a, f32x4 b)
{
    return _mm_castps_si128(_mm_cmpgt_ps(a, b));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline u32x4
compare_less_equal_f32x4(f32x4 a, f32x4 b)
{
    return _mm_castps_si128(_mm_cmple_ps(a, b));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline u32x4
compare_less_f32x4(f32x4 a, f32x4 b)
{
    return _mm_castps_si128(_mm_cmplt_ps(a, b));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
all_u32x4(u32x4 a)
{
    return _mm_movemask_epi8(a) == 0xF;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
all_u32x3(u32x4 a)
{
    return _mm_movemask_epi8(a) == 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
any_u32x4(u32x4 a)
{
    return _mm_movemask_epi8(a) != 0x0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
any_u32x3(u32x4 a)
{
    return (_mm_movemask_epi8(a) & 0x7) != 0x0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
load_unaligned_f32x3(const float* ptr)
{
    f32x4 vec = _mm_loadu_ps(ptr);
    return _mm_and_ps(vec, _mask_xyz);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
load_aligned_f32x3(const float* ptr)
{
    f32x4 vec = _mm_load_ps(ptr);
    return _mm_and_ps(vec, _mask_xyz);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
store_f32x3(f32x4 vec, float* ptr)
{
    f32x4 t1 = _mm_permute_ps(vec, _MM_SHUFFLE(1, 1, 1, 1));
    f32x4 t2 = _mm_permute_ps(vec, _MM_SHUFFLE(2, 2, 2, 2));
    _mm_store_ss(&ptr[0], vec);
    _mm_store_ss(&ptr[1], t1);
    _mm_store_ss(&ptr[2], t2);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
store_f32x4(f32x4 vec, float* ptr)
{
    _mm_store_ps(ptr, vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
store_f32(f32x4 vec, float* ptr)
{
    _mm_store_ss(ptr, vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
flip_sign_f32x4(f32x4 vec)
{
    static const __m128i _sign = _mm_setr_epi32(0x80000000, 0x80000000, 0x80000000, 0x80000000);
    return _mm_xor_ps(_mm_castsi128_ps(_sign), vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
mul_f32x4(f32x4 a, f32x4 b)
{
    return _mm_mul_ps(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
mul_first_f32x4(f32x4 a, f32x4 b)
{
    return _mm_mul_ss(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
fma_f32x4(f32x4 a, f32x4 b, f32x4 c)
{
#if NEBULA_MATH_FMA
    return _mm_fmadd_ps(a, b, c);
#else
    return _mm_add_ps(_mm_mul_ps(a, b),c);
#endif
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
div_f32x4(f32x4 a, f32x4 b)
{
    return _mm_div_ps(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
add_f32x4(f32x4 a, f32x4 b)
{
    return _mm_add_ps(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
sub_f32x4(f32x4 a, f32x4 b)
{
    return _mm_sub_ps(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
dot_f32x4(f32x4 a, f32x4 b)
{
    return _mm_dp_ps(a, b, 0xFF);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float
dot_f32x3(f32x4 a, f32x4 b)
{
    return _mm_cvtss_f32(_mm_dp_ps(a, b, 0x71));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
abs_f32x4(f32x4 a)
{
    unsigned int val = 0x7fffffff;
    f32x4 temp = _mm_set1_ps(*(float*)&val);
    return _mm_and_ps(a, temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float
get_first_f32x4(f32x4 a)
{
    return _mm_cvtss_f32(a);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
rcp_f32x4(f32x4 a)
{
    return _mm_rcp_ps(a);
}


//------------------------------------------------------------------------------
/**
    Two step sqrt
*/
__forceinline f32x4
rsqrt_f32x4(f32x4 a)
{
    return _mm_rsqrt_ps(a);
}

//------------------------------------------------------------------------------
/**
    Two step sqrt
*/
__forceinline f32x4
sqrt_f32x4(f32x4 a)
{
    return _mm_sqrt_ps(a);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
max_f32x4(f32x4 a, f32x4 b)
{
    return _mm_max_ps(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
max_first_f32x4(f32x4 a, f32x4 b)
{
    return _mm_max_ss(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
min_f32x4(f32x4 a, f32x4 b)
{
    return _mm_min_ps(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
min_first_f32x4(f32x4 a, f32x4 b)
{
    return _mm_min_ss(a, b);
}

//------------------------------------------------------------------------------
/**
*/
#define shuffle_f32x4(a, b, a0, a1, b0, b1) (_mm_shuffle_ps(a, b, _MM_SHUFFLE(b1, b0, a1, a0)))
//__forceinline f32x4
//shuffle_f32x4(f32x4 a, f32x4 b, uint8_t a0, uint8_t a1, uint8_t b0, uint8_t b1)
//{
//    return _mm_shuffle_ps(a, b, _MM_SHUFFLE(b1, b0, a1, a0));
//}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
convert_u32x4_to_f32x4(u32x4 a)
{
    // assumes u32x4 is never anything but a comparison
    return _mm_castsi128_ps(a);
}

#elif NEBULA_SIMD_AARCH64
#include <arm_neon.h>
typedef float32x4_t f32x4;
typedef int32x4_t i32x4;
typedef uint32x4_t u32x4;

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
set_f32x4(x, y, z, w)
{
    return f32x4{x,y,z,w}
}

//------------------------------------------------------------------------------
/**
*/
__forceinline i32x4
set_i32x4(x, y, z, w)
{
    return i32x4{x,y,z,w}
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
splat_f32x4(float x)
{
    return vdupq_n_f32(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
cast_i32x4_to_f32x4(i32x4 x)
{
    return vreinterpretq_s32_f32(x);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
set_last_f32x4(f32x4 v, float val)
{
    return vsetq_lane_f32(val, v, 3);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline u32x4
compare_equal_f32x4(f32x4 a, f32x4 b)
{
    return vceqq_f32(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline u32x4
compare_greater_equal_f32x4(f32x4 a, f32x4 b)
{
    return vcgeq_f32(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline u32x4
compare_greater_f32x4(f32x4 a, f32x4 b)
{
    return vcgtq_f32(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline u32x4
compare_less_equal_f32x4(f32x4 a, f32x4 b)
{
    return vcleq_f32(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline u32x4
compare_less_f32x4(f32x4 a, f32x4 b)
{
    return vcltq_f32(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
all_u32x4(u32x4 cmp)
{
    uint32x2_t low = vget_low_u32(cmp);
    uint32x2_t high = vget_high_u32(cmp);

    uint32x2_t and1 = vand_u32(low, high);
    uint32_t res = vget_lane_u32(and1, 0) & vget_lane_u32(and1, 1);

    return res == 0xFFFFFFFF;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
all_u32x3(u32x4 cmp)
{
    uint32x2_t low = vget_low_u32(cmp);
    uint32x2_t high = vget_high_u32(cmp);

    uint32x2_t and1 = vand_u32(low, vdup_n_u32(vgetq_lane_u32(cmp, 2)));
    uint32_t res = vget_lane_u32(and1, 0) & vget_lane_u32(and1, 1);

    return res == 0xFFFFFFFF;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
any_u32x4(u32x4 cmp)
{
    uint32x2_t low = vget_low_u32(cmp);
    uint32x2_t high = vget_high_u32(cmp);

    uint32x2_t and1 = vand_u32(low, high);
    uint32_t res = vget_lane_u32(and1, 0) & vget_lane_u32(and1, 1);

    return res != 0x0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
any_u32x3(u32x4 cmp)
{
    uint32x2_t low = vget_low_u32(cmp);
    uint32x2_t high = vget_high_u32(cmp);

    uint32x2_t and1 = vand_u32(low, vdup_n_u32(vgetq_lane_u32(cmp, 2)));
    uint32_t res = vget_lane_u32(and1, 0) & vget_lane_u32(and1, 1);

    return res == 0x0;
}


//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
load_unaligned_f32x3(const scalar* ptr)
{
    f32x4 vec = vld1q_f32(ptr);
    return set_last_f32x4(vec, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
load_aligned_f32x3(const scalar* ptr)
{
    f32x4 vec = vld1q_f32(ptr);
    return set_last_f32x4(vec, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
store_f32x3(f32x4 vec, scalar* ptr)
{
    ptr[0] = vgetq_lane_f32(vec, 0);
    ptr[1] = vgetq_lane_f32(vec, 1);
    ptr[2] = vgetq_lane_f32(vec, 2);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
store_f32x4(f32x4 vec, scalar* ptr)
{
    vst1q_f32(vec, ptr);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
store_f32(f32x4 vec, scalar* ptr)
{
    ptr[0] = vgetq_lane_f32(vec, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
flip_sign_f32x4(f32x4 vec)
{
    static const uint32x4_t sign_mask = vdupq_n_u32(0x80000000);
    return vreinterpretq_f32_u32(veorq_u32(vreinterpretq_u32_f32(vec), sign_mask));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
mul_f32x4(f32x4 a, f32x4 b)
{
    return vmulq_f32(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
mul_first_f32x4(f32x4 a, f32x4 b)
{
    float first = vget_lane_f32(a, 0) * vget_lane_f32(b, 0);
    return vsetq_lane_f32(first, a, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
fma_f32x4(f32x4 a, f32x4 b, f32x4 c)
{
    return vmlaq_f32(a, b, c);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
mad_f32x4(f32x4 a, f32x4 b, f32x4 c)
{
    return vmlaq_f32(a, b, c);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
div_f32x4(f32x4 a, f32x4 b)
{
    f32x4 recip = vrecpeq_f32(b);
    recip = vmulq_f32(recip, vrecpsq_f32(denominator, recip));
    recip = vmulq_f32(recip, vrecpsq_f32(denominator, recip));
    return vmulq_f32(a, recip);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
add_f32x4(f32x4 a, f32x4 b)
{
    return vaddq_f32(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
sub_f32x4(f32x4 a, f32x4 b)
{
    return vsubq_f32(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
dot_f32x4(f32x4 a, f32x4 b)
{
    f32x4 prod = vmulq(a, b);
    float32x2_t sum2 = vadd_f32(vget_low_f32(prod), vget_high_f32(prod));
    return vget_lane_f32(sum2, 0) + vget_lane_f32(sum2, 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
dot_f32x3(f32x4 a, f32x4 b)
{
    f32x4 prod = vmulq(a, b);
    float32x2_t low = vget_low_f32(prod); // get 0, 1
    float32x2_t sum2 = vpadd_f32(low, vdup_n_f32(vgetq_lane_f32(prod, 2))); // Add 0, 1 with splat of 2
    return vget_lane_f32(sum2, 0) + vget_lane_f32(sum2, 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
abs_f32x4(f32x4 a)
{
    return vabsq_f32(a);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
get_first_f32x4(f32x4 a)
{
    return vget_lane_f32(a, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
rcp_f32x4(f32x4 a)
{
    return vrecpeq_f32(a);
}

//------------------------------------------------------------------------------
/**
    Two step rsqrt
*/
__forceinline f32x4
rsqrt_f32x4(f32x4 a)
{
    f32x4 step = vrsqrteq_f32(a);
    step = vmulq_f32(step, vrsqrtsq_f32(a, step));
    step = vmulq_f32(step, vrsqrtsq_f32(a, step));
    return step;
}

//------------------------------------------------------------------------------
/**
    Two step sqrt
*/
__forceinline f32x4
sqrt_f32x4(f32x4 a)
{
    f32x4 rsqrt = rsqrt_f32x4(a);
    return vmulq_f32(a, rsqrt);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
max_f32x4(f32x4 a, f32x4 b)
{
    return vmaxq_f32(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
max_first_f32x4(f32x4 a, f32x4 b)
{
    float a0 = vget_lane_f32(a, 0);
    float b0 = vget_lane_f32(b, 0);
    float largest = a0 < b0 ? b0 : a0;
    return vsetq_lane_f32(largest, a, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
min_f32x4(f32x4 a, f32x4 b)
{
    return vminq_f32(a, b);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
min_first_f32x4(f32x4 a, f32x4 b)
{
    float a0 = vget_lane_f32(a, 0);
    float b0 = vget_lane_f32(b, 0);
    float smallest = a0 > b0 ? b0 : a0;
    return vsetq_lane_f32(smallest, a, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
shuffle_f32x4(f32x4 a, f32x4 b, uint8_t a0, uint8_t a1, uint8_t b0, uint8_t b1)
{
    return f32x4{
        vget_lane_f32(a, a0)
        , vget_lane_f32(a, a1)
        , vget_lane_f32(b, b0)
        , vget_lane_f32(b, b1)
    }
}

//------------------------------------------------------------------------------
/**
*/
__forceinline f32x4
convert_u32x4_to_f32x4(u32x4 a)
{
    return vcvtq_f32_u32(a);
}

#endif
