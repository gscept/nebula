#pragma once
//------------------------------------------------------------------------------
/**
    @struct Math::vec3

    A 3D vector.

    Internally represented as a __m128 for performance and alignment reasons.

    @see Math::vector
    @see Math::point

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/scalar.h"
#include <xmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <immintrin.h>

//------------------------------------------------------------------------------
namespace Math
{
struct mat4;
struct vec3;

static const __m128 _id_x = _mm_setr_ps(1.0f, 0.0f, 0.0f, 0.0f);
static const __m128 _id_y = _mm_setr_ps(0.0f, 1.0f, 0.0f, 0.0f);
static const __m128 _id_z = _mm_setr_ps(0.0f, 0.0f, 1.0f, 0.0f);
static const __m128 _id_w = _mm_setr_ps(0.0f, 0.0f, 0.0f, 1.0f);
static const __m128 _minus1 = _mm_setr_ps(-1.0f, -1.0f, -1.0f, -1.0f);
static const __m128 _plus1 = _mm_setr_ps(1.0f, 1.0f, 1.0f, 1.0f);
static const __m128i _sign = _mm_setr_epi32(0x80000000, 0x80000000, 0x80000000, 0x80000000);
static const __m128 _mask_xyz = _mm_castsi128_ps(_mm_setr_epi32( 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0 ));

struct NEBULA_ALIGN16 vec3
{
public:
    /// default constructor, NOTE: does NOT setup components!
    vec3() = default;
    /// construct from values
    vec3(scalar x, scalar y, scalar z);
    /// construct from single value
    explicit vec3(scalar v);
    /// copy constructor
    vec3(const vec3& rhs) = default;
    /// construct from SSE 128 byte float array
    vec3(const __m128& rhs);

    /// assign an vmVector4
    void operator=(const __m128& rhs);
    /// inplace add
    void operator+=(const vec3& rhs);
    /// inplace sub
    void operator-=(const vec3& rhs);
    /// inplace scalar multiply
    void operator*=(scalar s);
    /// muliply by a vector component-wise
    void operator*=(const vec3& rhs);
    /// divide by a vector component-wise
    void operator/=(const vec3& rhs);
    /// equality operator
    bool operator==(const vec3& rhs) const;
    /// inequality operator     
    bool operator!=(const vec3& rhs) const;

    /// load content from 16-byte-aligned memory
    void load(const scalar* ptr);
    /// load content from unaligned memory
    void loadu(const scalar* ptr);
    /// write content to 16-byte-aligned memory through the write cache
    void store(scalar* ptr) const;
    /// write content to unaligned memory through the write cache
    void storeu(scalar* ptr) const;
    /// stream content to 16-byte-aligned memory circumventing the write-cache
    void stream(scalar* ptr) const;

    /// set content
    void set(scalar x, scalar y, scalar z);

    /// read-only access to indexed component
    scalar& operator[](const int index);
    /// read-only access to indexed component
    scalar operator[](const int index) const;

    union
    {
        struct
        {
            // we can access __w to check it, but we don't actually use it
            float x, y, z, __w;
        };
        __m128 vec;
        float v[3];
    };
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
vec3::vec3(scalar x, scalar y, scalar z)
{
    this->vec = _mm_setr_ps(x, y, z, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vec3::vec3(scalar v)
{
    this->vec = _mm_setr_ps(v, v, v, 0.0f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vec3::vec3(const __m128& rhs)
{
    this->vec = _mm_insert_ps(rhs, _id_w, 0b111000);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec3::operator=(const __m128& rhs)
{
    this->vec = _mm_insert_ps(rhs, _id_w, 0b111000);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
vec3::operator==(const vec3& rhs) const
{
    __m128 vTemp = _mm_cmpeq_ps(this->vec, rhs.vec);
    return ((_mm_movemask_ps(vTemp)==0x0f) != 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
vec3::operator!=(const vec3 &rhs) const
{
    __m128 vTemp = _mm_cmpeq_ps(this->vec, rhs.vec);
    return ((_mm_movemask_ps(vTemp)==0x0f) == 0);
}

//------------------------------------------------------------------------------
/**
    Load 4 floats from 16-byte-aligned memory.
*/
__forceinline void
vec3::load(const scalar* ptr)
{
    this->vec = _mm_load_ps(ptr);
    this->vec = _mm_and_ps(this->vec, _mask_xyz);
}

//------------------------------------------------------------------------------
/**
    Load 4 floats from unaligned memory.
*/
__forceinline void
vec3::loadu(const scalar* ptr)
{
    this->vec = _mm_loadu_ps(ptr);
    this->vec = _mm_and_ps(this->vec, _mask_xyz);
}

//------------------------------------------------------------------------------
/**
    Store to 16-byte-aligned float pointer.
*/
__forceinline void
vec3::store(scalar* ptr) const
{
    __m128 vv = _mm_permute_ps(this->vec, _MM_SHUFFLE(2, 2, 2, 2));
    _mm_storel_epi64(reinterpret_cast<__m128i*>(ptr), _mm_castps_si128(this->vec));
    _mm_store_ss(&ptr[2], vv);
}

//------------------------------------------------------------------------------
/**
    Store to non-aligned float pointer.
*/
__forceinline void
vec3::storeu(scalar* ptr) const
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
vec3::stream(scalar* ptr) const
{
    this->store(ptr);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
operator-(const vec3& lhs)
{
    return vec3(_mm_xor_ps(_mm_castsi128_ps(_sign), lhs.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
operator*(const vec3& lhs, scalar t)
{
    __m128 temp = _mm_set1_ps(t);
    return _mm_mul_ps(lhs.vec, temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
operator*(const vec3& lhs, const vec3& rhs)
{
    return _mm_mul_ps(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec3::operator*=(const vec3& rhs)
{
    this->vec = _mm_mul_ps(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec3::operator/=( const vec3& rhs )
{
    this->vec = _mm_div_ps(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec3::operator+=(const vec3 &rhs)
{
    this->vec = _mm_add_ps(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec3::operator-=(const vec3 &rhs)
{
    this->vec = _mm_sub_ps(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec3::operator*=(scalar s)
{
    __m128 temp = _mm_set1_ps(s);
    this->vec = _mm_mul_ps(this->vec, temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
operator+(const vec3& lhs, const vec3 &rhs)
{
    return _mm_add_ps(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
operator-(const vec3& lhs, const vec3& rhs)
{
    return _mm_sub_ps(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec3::set(scalar x, scalar y, scalar z)
{
    this->vec = _mm_setr_ps(x, y, z, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar&
vec3::operator[]( const int index )
{
    n_assert(index < 3);
    return this->v[index];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
vec3::operator[](const int index) const
{
    n_assert(index < 3);
    return this->v[index];
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
length(const vec3& v)
{
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(v.vec, v.vec, 0x71)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
lengthsq(const vec3& v)
{
    return _mm_cvtss_f32(_mm_dp_ps(v.vec, v.vec, 0x71));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
reciprocal(const vec3& v)
{
    return _mm_div_ps(_plus1, v.vec);   
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
reciprocalapprox(const vec3& v)
{
    return _mm_rcp_ps(v.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
multiply(const vec3& v0, const vec3& v1)
{
    return _mm_mul_ps(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
multiplyadd( const vec3& v0, const vec3& v1, const vec3& v2 )
{
#if NEBULA_MATH_FMA
    return _mm_fmadd_ps(v0.vec, v1.vec, v2.vec);
#else
    return _mm_add_ps(_mm_mul_ps(v0.vec, v1.vec),v2.vec);
#endif
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3 
divide(const vec3& v0, const vec3& v1)
{
    return _mm_div_ps(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
abs(const vec3& v)
{
    unsigned int val = 0x7fffffff;
    __m128 temp = _mm_set1_ps(*(float*)&val);
    return _mm_and_ps(v.vec, temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
cross(const vec3& v0, const vec3& v1)
{
    __m128 tmp0, tmp1, tmp2, tmp3, result;
    tmp0 = _mm_shuffle_ps( v0.vec, v0.vec, _MM_SHUFFLE(3,0,2,1) );
    tmp1 = _mm_shuffle_ps( v1.vec, v1.vec, _MM_SHUFFLE(3,1,0,2) );
    tmp2 = _mm_shuffle_ps( v0.vec, v0.vec, _MM_SHUFFLE(3,1,0,2) );
    tmp3 = _mm_shuffle_ps( v1.vec, v1.vec, _MM_SHUFFLE(3,0,2,1) );
    result = _mm_mul_ps( tmp0, tmp1 );
    result = _mm_sub_ps( result, _mm_mul_ps( tmp2, tmp3 ) );
    return result;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
dot(const vec3& v0, const vec3& v1)
{
    return _mm_cvtss_f32(_mm_dp_ps(v0.vec, v1.vec, 0x71));
}

//------------------------------------------------------------------------------
/**
    calculates Result = v0 + f * (v1 - v0) + g * (v2 - v0)
*/
__forceinline vec3
barycentric(const vec3& v0, const vec3 &v1, const vec3 &v2, scalar f, scalar g)
{
    __m128 R1 = _mm_sub_ps(v1.vec,v0.vec);
    __m128 SF = _mm_set_ps1(f);
    __m128 R2 = _mm_sub_ps(v2.vec,v0.vec);
    __m128 SG = _mm_set_ps1(g);
    R1 = _mm_mul_ps(R1,SF);
    R2 = _mm_mul_ps(R2,SG);
    R1 = _mm_add_ps(R1,v0.vec);
    R1 = _mm_add_ps(R1,R2);
    return R1;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
catmullrom(const vec3& v0, const vec3& v1, const vec3& v2, const vec3& v3, scalar s)
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
    P0 = _mm_add_ps(P0,P1);
    P2 = _mm_add_ps(P2,P3);
    P0 = _mm_add_ps(P0,P2);
    return P0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
hermite(const vec3& v1, const vec3& t1, const vec3& v2, const vec3& t2, scalar s)
{
    scalar s2 = s * s;
    scalar s3 = s * s2;

    __m128 P0 = _mm_set_ps1(2.0f * s3 - 3.0f * s2 + 1.0f);
    __m128 T0 = _mm_set_ps1(s3 - 2.0f * s2 + s);
    __m128 P1 = _mm_set_ps1(-2.0f * s3 + 3.0f * s2);
    __m128 T1 = _mm_set_ps1(s3 - s2);

    __m128 vResult = _mm_mul_ps(P0, v1.vec);
    __m128 vTemp = _mm_mul_ps(T0, t1.vec);
    vResult = _mm_add_ps(vResult,vTemp);
    vTemp = _mm_mul_ps(P1, v2.vec);
    vResult = _mm_add_ps(vResult,vTemp);
    vTemp = _mm_mul_ps(T1, t2.vec);
    vResult = _mm_add_ps(vResult,vTemp);
    return vResult;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
angle(const vec3& v0, const vec3& v1)
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
    return acos(cangle);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
lerp(const vec3& v0, const vec3& v1, scalar s)
{
    return v0 + ((v1-v0) * s);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
maximize(const vec3& v0, const vec3& v1)
{
    return _mm_max_ps(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
minimize(const vec3& v0, const vec3& v1)
{
    return _mm_min_ps(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
clamp(const vec3& clamp, const vec3& min, const vec3& max)
{
    __m128 temp = _mm_max_ps(min.vec, clamp.vec);
    temp = _mm_min_ps(temp, max.vec);
    return vec3(temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
normalize(const vec3& v)
{
    if (v == vec3(0)) return v;
    __m128 t = _mm_div_ps(v.vec, _mm_sqrt_ps(_mm_dp_ps(v.vec, v.vec, 0x77)));
    return _mm_insert_ps(t, v.vec, 0xF0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
normalizeapprox(const vec3& v)
{
    if (v == vec3(0)) return v;
    __m128 t = _mm_rsqrt_ps(_mm_dp_ps(v.vec, v.vec, 0x7f));
    t = _mm_or_ps(t, _id_w);
    return _mm_mul_ps(v.vec, t);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
reflect(const vec3& normal, const vec3& incident)
{
    __m128 res = _mm_mul_ps(incident.vec, normal.vec);
    res = _mm_add_ps(_mm_shuffle_ps(res, res, _MM_SHUFFLE(0,0,0,0)),
        _mm_add_ps(_mm_shuffle_ps(res, res, _MM_SHUFFLE(1,1,1,1)), _mm_shuffle_ps(res, res, _MM_SHUFFLE(2,2,2,2))));
    res = _mm_add_ps(res, res);
    res = _mm_mul_ps(res, normal.vec);
    res = _mm_sub_ps(incident.vec,res);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
less_any(const vec3& v0, const vec3& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
less_all(const vec3& v0, const vec3& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res == 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
lessequal_any(const vec3& v0, const vec3& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
lessequal_all(const vec3& v0, const vec3& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res == 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greater_any(const vec3& v0, const vec3& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greater_all(const vec3& v0, const vec3& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res == 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greaterequal_any(const vec3& v0, const vec3& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greaterequal_all(const vec3& v0, const vec3& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res == 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
equal_any(const vec3& v0, const vec3& v1)
{
    __m128 vTemp = _mm_cmpeq_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
nearequal(const vec3& v0, const vec3& v1, const vec3& epsilon)
{
    __m128 delta = _mm_sub_ps(v0.vec, v1.vec);
    __m128 temp = _mm_setzero_ps();
    temp = _mm_sub_ps(temp, delta);
    temp = _mm_max_ps(temp, delta);
    temp = _mm_cmple_ps(temp, epsilon.vec);
    temp = _mm_and_ps(temp, _mask_xyz);
    return (_mm_movemask_ps(temp) == 0x7) != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
less(const vec3& v0, const vec3& v1)
{
    return _mm_min_ps(_mm_cmplt_ps(v0.vec, v1.vec), _plus1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
greater(const vec3& v0, const vec3& v1)
{
    return _mm_min_ps(_mm_cmpgt_ps(v0.vec, v1.vec), _plus1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
equal(const vec3& v0, const vec3& v1)
{
    return _mm_min_ps(_mm_cmpeq_ps(v0.vec, v1.vec), _plus1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
splat(const vec3& v, uint element)
{
    n_assert(element < 3 && element >= 0);

    __m128 res;
    switch (element)
    {
    case 0:
        res = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(0, 0, 0, 0));
        break;
    case 1:
        res = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(1, 1, 1, 1));
        break;
    case 2:
        res = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(2, 2, 2, 2));
        break;
    }
    res = _mm_and_ps(res, _mask_xyz);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
splat_x(const vec3& v)
{
    __m128 res = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(0, 0, 0, 0));
    res = _mm_and_ps(res, _mask_xyz);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
splat_y(const vec3& v)
{
    __m128 res = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(1, 1, 1, 1));
    res = _mm_and_ps(res, _mask_xyz);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
splat_z(const vec3& v)
{
    __m128 res = _mm_shuffle_ps(v.vec, v.vec, _MM_SHUFFLE(2, 2, 2, 2));
    res = _mm_and_ps(res, _mask_xyz);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
permute(const vec3& v0, const vec3& v1, unsigned int i0, unsigned int i1, unsigned int i2)
{
    static __m128i three = _mm_set_epi32(3,3,3,3);

    NEBULA_ALIGN16 unsigned int elem[4] = { i0, i1, i2, 7 };
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
__forceinline vec3
select(const vec3& v0, const vec3& v1, const uint i0, const uint i1, const uint i2)
{
    //FIXME this should be converted to something similiar as XMVectorSelect
    return permute(v0, v1, i0, i1, i2);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
select(const vec3& v0, const vec3& v1, const vec3& control)
{
    __m128 v0masked = _mm_andnot_ps(control.vec, v0.vec);
    __m128 v1masked = _mm_and_ps(v1.vec, control.vec);
    return _mm_or_ps(v0masked, v1masked);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
floor(const vec3& v)
{
    return _mm_floor_ps(v.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
ceiling(const vec3& v)
{
    return _mm_ceil_ps(v.vec);
}

} // namespace Math
//------------------------------------------------------------------------------








