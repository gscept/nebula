#pragma once
//------------------------------------------------------------------------------
/**
    @struct Math::vec4

    A 4D vector.

    @see Math::vector
    @see Math::point

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/scalar.h"
#include "vec3.h"

//------------------------------------------------------------------------------
namespace Math
{
struct mat4;

struct NEBULA_ALIGN16 vec4
{
public:
    /// default constructor, NOTE: does NOT setup components!
    vec4() = default;
    /// construct from values
    vec4(scalar x, scalar y, scalar z, scalar w);
    /// construct from single value
    explicit vec4(scalar v);
    /// copy constructor
    vec4(const vec4& rhs) = default;
    /// copy constructor from vec3
    vec4(const vec3& rhs, float w);
    /// construct from SSE 128 byte float array
    vec4(const __m128& rhs);

    /// assign an vmVector4
    void operator=(const __m128 &rhs);
    /// inplace add
    void operator+=(const vec4 &rhs);
    /// inplace sub
    void operator-=(const vec4 &rhs);
    /// inplace scalar multiply
    void operator*=(scalar s);
    /// muliply by a vector component-wise
    void operator*=(const vec4& rhs);
    /// divide by a vector component-wise
    void operator/=(const vec4& rhs);
    /// equality operator
    bool operator==(const vec4 &rhs) const;
    /// inequality operator
    bool operator!=(const vec4 &rhs) const;

    /// load content from 16-byte-aligned memory
    void load(const scalar* ptr);
    /// load content from unaligned memory
    void loadu(const scalar* ptr);
    /// write content to 16-byte-aligned memory through the write cache
    void store(scalar* ptr) const;
    /// write content to unaligned memory through the write cache
    void storeu(scalar* ptr) const;
    /// write content to 16-byte-aligned memory through the write cache
    void store3(scalar* ptr) const;
    /// write content to unaligned memory through the write cache
    void storeu3(scalar* ptr) const;
    /// stream content to 16-byte-aligned memory circumventing the write-cache
    void stream(scalar* ptr) const;

    /// load 3 floats into x,y,z from unaligned memory
    void load_float3(const void* ptr, float w);
    /// load from UByte4N packed vector
    void load_ubyte4n(const void* ptr);
    /// load from Byte4N packed vector
    void load_byte4n(const void* ptr);
    /// set content
    void set(scalar x, scalar y, scalar z, scalar w);

    /// swizzle vector
    template<int X, int Y, int Z, int W>
    vec4 swizzle(const vec4& v);

    /// read-only access to indexed component
    scalar& operator[](const int index);
    /// read-only access to indexed component
    scalar operator[](const int index) const;
    /// implicit vec3 conversion operator
    operator vec3() const { return Math::vec3(this->x, this->y, this->z); }

    union
    {
        struct
        {
            float x, y, z, w;
        };
        __m128 vec;
        float v[4];
    };
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
vec4::vec4(scalar x, scalar y, scalar z, scalar w)
{
    this->vec = _mm_setr_ps(x, y, z, w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vec4::vec4(scalar v)
{
    this->vec = _mm_set1_ps(v);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vec4::vec4(const __m128& rhs)
{
    this->vec = rhs;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vec4::vec4(const vec3& rhs, float w)
{
    this->vec = rhs.vec;
    this->w = w;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec4::operator=(const __m128& rhs)
{
    this->vec = rhs;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
vec4::operator==(const vec4& rhs) const
{
    __m128 vTemp = _mm_cmpeq_ps(this->vec, rhs.vec);
    return ((_mm_movemask_ps(vTemp) == 0x0f) != 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
vec4::operator!=(const vec4& rhs) const
{
    __m128 vTemp = _mm_cmpeq_ps(this->vec, rhs.vec);
    return ((_mm_movemask_ps(vTemp) == 0x0f) == 0);
}

//------------------------------------------------------------------------------
/**
    Load 4 floats from 16-byte-aligned memory.
*/
__forceinline void
vec4::load(const scalar* ptr)
{
    this->vec = _mm_load_ps(ptr);
}

//------------------------------------------------------------------------------
/**
    Load 4 floats from unaligned memory.
*/
__forceinline void
vec4::loadu(const scalar* ptr)
{
    this->vec = _mm_loadu_ps(ptr);
}

//------------------------------------------------------------------------------
/**
    Store to 16-byte-aligned float pointer.
*/
__forceinline void
vec4::store(scalar* ptr) const
{
    _mm_store_ps(ptr, this->vec);
}

//------------------------------------------------------------------------------
/**
    Store to non-aligned float pointer.
*/
__forceinline void
vec4::storeu(scalar* ptr) const
{
    _mm_storeu_ps(ptr, this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void 
vec4::store3(scalar* ptr) const
{
    __m128 vv = _mm_permute_ps(this->vec, _MM_SHUFFLE(2, 2, 2, 2));
    _mm_storel_epi64(reinterpret_cast<__m128i*>(ptr), _mm_castps_si128(this->vec));
    _mm_store_ss(&ptr[2], vv);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void 
vec4::storeu3(scalar* ptr) const
{
    __m128 t1 = _mm_permute_ps(this->vec, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 t2 = _mm_permute_ps(this->vec, _MM_SHUFFLE(2, 2, 2, 2));
    _mm_store_ss(&ptr[0], this->vec);
    _mm_store_ss(&ptr[1], t1);
    _mm_store_ss(&ptr[2], t2);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec4::stream(scalar* ptr) const
{
    this->store(ptr);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec4::load_float3(const void* ptr, float w)
{
    float* source = (float*)ptr;
    this->vec = _mm_load_ps(source);
    this->w = w;
}


//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec4::operator*=(const vec4& rhs)
{
    this->vec = _mm_mul_ps(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec4::operator/=(const vec4& rhs)
{
    this->vec = _mm_div_ps(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec4::operator+=(const vec4& rhs)
{
    this->vec = _mm_add_ps(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec4::operator-=(const vec4& rhs)
{
    this->vec = _mm_sub_ps(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec4::operator*=(scalar s)
{
    __m128 temp = _mm_set1_ps(s);
    this->vec = _mm_mul_ps(this->vec, temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec4::set(scalar x, scalar y, scalar z, scalar w)
{
    this->vec = _mm_setr_ps(x, y, z, w);
}

//------------------------------------------------------------------------------
/**
*/
template<int X, int Y, int Z, int W>
inline vec4 
vec4::swizzle(const vec4& v)
{
    return _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(W, Z, Y, X));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
vec4::operator[](const int index)
{
    n_assert(index < 4);
    return this->v[index];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
vec4::operator[](const int index) const
{
    n_assert(index < 4);
    return this->v[index];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
operator-(const vec4& lhs)
{
    return vec4(_mm_xor_ps(_mm_castsi128_ps(_sign), lhs.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
operator*(const vec4& lhs, scalar t)
{
    __m128 temp = _mm_set1_ps(t);
    return _mm_mul_ps(lhs.vec, temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
operator*(const vec4& lhs, const vec4& rhs)
{
    return _mm_mul_ps(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
operator+(const vec4& lhs, const vec4& rhs)
{
    return _mm_add_ps(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
operator-(const vec4& lhs, const vec4& rhs)
{
    return _mm_sub_ps(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
length(const vec4& v)
{
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(v.vec, v.vec, 0xF1)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
length3(const vec4& v)
{
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(v.vec, v.vec, 0x71)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
lengthsq(const vec4& v)
{
    return _mm_cvtss_f32(_mm_dp_ps(v.vec, v.vec, 0xF1));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
lengthsq3(const vec4& v)
{
    return _mm_cvtss_f32(_mm_dp_ps(v.vec, v.vec, 0x71));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
reciprocal(const vec4& v)
{
    return _mm_div_ps(_plus1, v.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
reciprocalapprox(const vec4& v)
{
    return _mm_rcp_ps(v.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
multiply(const vec4& v0, const vec4& v1)
{
    return _mm_mul_ps(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
multiplyadd(const vec4& v0, const vec4& v1, const vec4& v2)
{
#if NEBULA_MATH_FMA
    return _mm_fmadd_ps(v0.vec, v1.vec, v2.vec);
#else
    return _mm_add_ps(_mm_mul_ps(v0.vec, v1.vec), v2.vec);
#endif
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
divide(const vec4& v0, const vec4& v1)
{
    return _mm_div_ps(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
abs(const vec4& v)
{
    unsigned int val = 0x7fffffff;
    __m128 temp = _mm_set1_ps(*(float*)&val);
    return _mm_and_ps(v.vec, temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
cross3(const vec4& v0, const vec4& v1)
{
    __m128 tmp0, tmp1, tmp2, tmp3, result;
    tmp0 = _mm_shuffle_ps(v0.vec, v0.vec, _MM_SHUFFLE(3, 0, 2, 1));
    tmp1 = _mm_shuffle_ps(v1.vec, v1.vec, _MM_SHUFFLE(3, 1, 0, 2));
    tmp2 = _mm_shuffle_ps(v0.vec, v0.vec, _MM_SHUFFLE(3, 1, 0, 2));
    tmp3 = _mm_shuffle_ps(v1.vec, v1.vec, _MM_SHUFFLE(3, 0, 2, 1));
    result = _mm_mul_ps(tmp0, tmp1);
    result = _mm_sub_ps(result, _mm_mul_ps(tmp2, tmp3));
    return result;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
dot(const vec4& v0, const vec4& v1)
{
    return _mm_cvtss_f32(_mm_dp_ps(v0.vec, v1.vec, 0xF1));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
dot3(const vec4& v0, const vec4& v1)
{
    return _mm_cvtss_f32(_mm_dp_ps(v0.vec, v1.vec, 0x71));
}

//------------------------------------------------------------------------------
/**
    calculates Result = v0 + f * (v1 - v0) + g * (v2 - v0)
*/
__forceinline vec4
barycentric(const vec4& v0, const vec4& v1, const vec4& v2, scalar f, scalar g)
{
    __m128 R1 = _mm_sub_ps(v1.vec, v0.vec);
    __m128 SF = _mm_set_ps1(f);
    __m128 R2 = _mm_sub_ps(v2.vec, v0.vec);
    __m128 SG = _mm_set_ps1(g);
    R1 = _mm_mul_ps(R1, SF);
    R2 = _mm_mul_ps(R2, SG);
    R1 = _mm_add_ps(R1, v0.vec);
    R1 = _mm_add_ps(R1, R2);
    return R1;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
catmullrom(const vec4& v0, const vec4& v1, const vec4& v2, const vec4& v3, scalar s)
{
    scalar s2 = s * s;
    scalar s3 = s * s2;

    __m128 P0 = _mm_set_ps1((-s3 + 2.0f * s2 - s) * 0.5f);
    __m128 P1 = _mm_set_ps1((3.0f * s3 - 5.0f * s2 + 2.0f) * 0.5f);
    __m128 P2 = _mm_set_ps1((-3.0f * s3 + 4.0f * s2 + s) * 0.5f);
    __m128 P3 = _mm_set_ps1((s3 - s2) * 0.5f);

    P0 = _mm_mul_ps(P0, v0.vec);
    P1 = _mm_mul_ps(P1, v1.vec);
    P2 = _mm_mul_ps(P2, v2.vec);
    P3 = _mm_mul_ps(P3, v3.vec);
    P0 = _mm_add_ps(P0, P1);
    P2 = _mm_add_ps(P2, P3);
    P0 = _mm_add_ps(P0, P2);
    return P0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
hermite(const vec4& v1, const vec4& t1, const vec4& v2, const vec4& t2, scalar s)
{
    scalar s2 = s * s;
    scalar s3 = s * s2;

    __m128 P0 = _mm_set_ps1(2.0f * s3 - 3.0f * s2 + 1.0f);
    __m128 T0 = _mm_set_ps1(s3 - 2.0f * s2 + s);
    __m128 P1 = _mm_set_ps1(-2.0f * s3 + 3.0f * s2);
    __m128 T1 = _mm_set_ps1(s3 - s2);

    __m128 vResult = _mm_mul_ps(P0, v1.vec);
    __m128 vTemp = _mm_mul_ps(T0, t1.vec);
    vResult = _mm_add_ps(vResult, vTemp);
    vTemp = _mm_mul_ps(P1, v2.vec);
    vResult = _mm_add_ps(vResult, vTemp);
    vTemp = _mm_mul_ps(T1, t2.vec);
    vResult = _mm_add_ps(vResult, vTemp);
    return vResult;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
angle(const vec4& v0, const vec4& v1)
{

    __m128 l0 = _mm_mul_ps(v0.vec, v0.vec);
    l0 = _mm_add_ps(_mm_shuffle_ps(l0, l0, _MM_SHUFFLE(0, 0, 0, 0)),
        _mm_add_ps(_mm_shuffle_ps(l0, l0, _MM_SHUFFLE(1, 1, 1, 1)), _mm_shuffle_ps(l0, l0, _MM_SHUFFLE(2, 2, 2, 2))));

    __m128 l1 = _mm_mul_ps(v1.vec, v1.vec);
    l1 = _mm_add_ps(_mm_shuffle_ps(l1, l1, _MM_SHUFFLE(0, 0, 0, 0)),
        _mm_add_ps(_mm_shuffle_ps(l1, l1, _MM_SHUFFLE(1, 1, 1, 1)), _mm_shuffle_ps(l1, l1, _MM_SHUFFLE(2, 2, 2, 2))));

    __m128 l = _mm_shuffle_ps(l0, l1, _MM_SHUFFLE(0, 0, 0, 0));
    l = _mm_rsqrt_ps(l);
    l = _mm_mul_ss(_mm_shuffle_ps(l, l, _MM_SHUFFLE(0, 0, 0, 0)), _mm_shuffle_ps(l, l, _MM_SHUFFLE(1, 1, 1, 1)));


    __m128 dot = _mm_mul_ps(v0.vec, v1.vec);
    dot = _mm_add_ps(_mm_shuffle_ps(dot, dot, _MM_SHUFFLE(0, 0, 0, 0)),
        _mm_add_ps(_mm_shuffle_ps(dot, dot, _MM_SHUFFLE(1, 1, 1, 1)),
            _mm_add_ps(_mm_shuffle_ps(dot, dot, _MM_SHUFFLE(2, 2, 2, 2)), _mm_shuffle_ps(dot, dot, _MM_SHUFFLE(3, 3, 3, 3)))));

    dot = _mm_mul_ss(dot, l);

    dot = _mm_max_ss(dot, _minus1);
    dot = _mm_min_ss(dot, _plus1);

    scalar cangle;
    _mm_store_ss(&cangle, dot);
    return Math::acos(cangle);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
lerp(const vec4& v0, const vec4& v1, scalar s)
{
    return v0 + ((v1 - v0) * s);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
maximize(const vec4& v0, const vec4& v1)
{
    return _mm_max_ps(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
minimize(const vec4& v0, const vec4& v1)
{
    return _mm_min_ps(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
clamp(const vec4& clamp, const vec4& min, const vec4& max)
{
    __m128 temp = _mm_max_ps(min.vec, clamp.vec);
    temp = _mm_min_ps(temp, max.vec);
    return vec4(temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
normalize(const vec4& v)
{
    if (v == vec4(0)) return v;
    return _mm_div_ps(v.vec, _mm_sqrt_ps(_mm_dp_ps(v.vec, v.vec, 0xFF)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
normalizeapprox(const vec4& v)
{
    if (v == vec4(0)) return v;
    return _mm_mul_ps(v.vec, _mm_rsqrt_ps(_mm_dp_ps(v.vec, v.vec, 0xFF)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
normalize3(const vec4& v)
{
    if (v == vec4(0)) return v;
    __m128 t = _mm_div_ps(v.vec, _mm_sqrt_ps(_mm_dp_ps(v.vec, v.vec, 0x77)));
    return _mm_insert_ps(t, v.vec, 0xF0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
normalize3approx(const vec4& v)
{
    if (v == vec4(0)) return v;
    __m128 t = _mm_mul_ps(v.vec, _mm_rsqrt_ps(_mm_dp_ps(v.vec, v.vec, 0x77)));
    return _mm_insert_ps(t, v.vec, 0xF0);
}
//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
reflect(const vec4& normal, const vec4& incident)
{
    __m128 res = _mm_mul_ps(incident.vec, normal.vec);
    res = _mm_add_ps(_mm_shuffle_ps(res, res, _MM_SHUFFLE(0, 0, 0, 0)),
        _mm_add_ps(_mm_shuffle_ps(res, res, _MM_SHUFFLE(1, 1, 1, 1)), _mm_shuffle_ps(res, res, _MM_SHUFFLE(2, 2, 2, 2))));
    res = _mm_add_ps(res, res);
    res = _mm_mul_ps(res, normal.vec);
    res = _mm_sub_ps(incident.vec, res);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
perspective_div(const vec4& v)
{
    __m128 d = _mm_set_ps1(1.0f / v.w);
    return _mm_mul_ps(v.vec, d);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
less_any(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp);
    return res != 0xf;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
less_all(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp);
    return res == 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
lessequal_any(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp);
    return res != 0xf;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
lessequal_all(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp);
    return res == 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greater_any(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp);
    return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greater_all(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp);
    return res == 0xf;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greaterequal_any(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp);
    return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greaterequal_all(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp);
    return res == 0xf;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
equal_any(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpeq_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp);
    return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
nearequal(const vec4& v0, const vec4& v1, const float epsilon)
{
    __m128 eps = _mm_set1_ps(epsilon);
    __m128 delta = _mm_sub_ps(v0.vec, v1.vec);
    __m128 temp = _mm_setzero_ps();
    temp = _mm_sub_ps(temp, delta);
    temp = _mm_max_ps(temp, delta);
    temp = _mm_cmple_ps(temp, eps);
    return (_mm_movemask_ps(temp) == 0xf) != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
nearequal(const vec4& v0, const vec4& v1, const vec4& epsilon)
{
    __m128 delta = _mm_sub_ps(v0.vec, v1.vec);
    __m128 temp = _mm_setzero_ps();
    temp = _mm_sub_ps(temp, delta);
    temp = _mm_max_ps(temp, delta);
    temp = _mm_cmple_ps(temp, epsilon.vec);
    return (_mm_movemask_ps(temp) == 0xf) != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
less(const vec4& v0, const vec4& v1)
{
    return _mm_min_ps(_mm_cmplt_ps(v0.vec, v1.vec), _plus1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
greater(const vec4& v0, const vec4& v1)
{
    return _mm_min_ps(_mm_cmpgt_ps(v0.vec, v1.vec), _plus1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
equal(const vec4& v0, const vec4& v1)
{
    return _mm_min_ps(_mm_cmpeq_ps(v0.vec, v1.vec), _plus1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
less3_any(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
less3_all(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res == 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
lessequal3_any(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
lessequal3_all(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res == 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greater3_any(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greater3_all(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res == 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greaterequal3_any(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greaterequal3_all(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res == 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
equal3_any(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpeq_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
equal3_all(const vec4& v0, const vec4& v1)
{
    __m128 vTemp = _mm_cmpeq_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res == 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
nearequal3(const vec4& v0, const vec4& v1, const vec4& epsilon)
{
    __m128 delta = _mm_sub_ps(v0.vec, v1.vec);
    __m128 temp = _mm_setzero_ps();
    temp = _mm_sub_ps(temp, delta);
    temp = _mm_max_ps(temp, delta);
    temp = _mm_cmple_ps(temp, epsilon.vec);
    return (_mm_movemask_ps(temp) == 0x7) != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
splat(const vec4& v, uint element)
{
    n_assert(element < 4 && element >= 0);

    switch (element)
    {
    case 0:
        return _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(0, 0, 0, 0));
    case 1:
        return _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(1, 1, 1, 1));
    case 2:
        return _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(2, 2, 2, 2));
    }
    return _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(3, 3, 3, 3));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
splat_x(const vec4& v)
{
    return _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(0, 0, 0, 0));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
splat_y(const vec4& v)
{
    return _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(1, 1, 1, 1));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
splat_z(const vec4& v)
{
    return _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(2, 2, 2, 2));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
splat_w(const vec4& v)
{
    return _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(3, 3, 3, 3));
}

static const unsigned int PERMUTE_0X = 0;
static const unsigned int PERMUTE_0Y = 1;
static const unsigned int PERMUTE_0Z = 2;
static const unsigned int PERMUTE_0W = 3;
static const unsigned int PERMUTE_1X = 4;
static const unsigned int PERMUTE_1Y = 5;
static const unsigned int PERMUTE_1Z = 6;
static const unsigned int PERMUTE_1W = 7;
//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
permute(const vec4& v0, const vec4& v1, unsigned int i0, unsigned int i1, unsigned int i2, unsigned int i3)
{
    static __m128i three = _mm_set_epi32(3, 3, 3, 3);

    NEBULA_ALIGN16 unsigned int elem[4] = { i0, i1, i2, i3 };
    __m128i vControl = _mm_load_si128(reinterpret_cast<const __m128i*>(&elem[0]));

    __m128i vSelect = _mm_cmpgt_epi32(vControl, three);
    vControl = _mm_and_si128(vControl, three);

    __m128 shuffled1 = _mm_permutevar_ps(v0.vec, vControl);
    __m128 shuffled2 = _mm_permutevar_ps(v1.vec, vControl);

    __m128 masked1 = _mm_andnot_ps(_mm_castsi128_ps(vSelect), shuffled1);
    __m128 masked2 = _mm_and_ps(_mm_castsi128_ps(vSelect), shuffled2);

    return _mm_or_ps(masked1, masked2);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
select(const vec4& v0, const vec4& v1, const uint i0, const uint i1, const uint i2, const uint i3)
{
    //FIXME this should be converted to something similiar as XMVectorSelect
    return permute(v0, v1, i0, i1, i2, i3);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
select(const vec4& v0, const vec4& v1, const vec4& control)
{
    __m128 v0masked = _mm_andnot_ps(control.vec, v0.vec);
    __m128 v1masked = _mm_and_ps(v1.vec, control.vec);
    return _mm_or_ps(v0masked, v1masked);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
floor(const vec4& v)
{
    return _mm_floor_ps(v.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec4
ceil(const vec4& v)
{
    return _mm_ceil_ps(v.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
xyz(const vec4& v)
{
    vec3 res;
    res.vec = _mm_and_ps(v.vec, _mask_xyz);
    return res;
}

} // namespace Math
//------------------------------------------------------------------------------
