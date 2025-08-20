#pragma once
//------------------------------------------------------------------------------
/**
    @struct Math::vec3

    A 3D vector.

    Internally represented as a f32x4 for performance and alignment reasons.

    @see Math::vector
    @see Math::point

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/scalar.h"
#include "core/simd.h"

//------------------------------------------------------------------------------
namespace Math
{
struct mat4;
struct vec3;

static const f32x4 _id_x = set_f32x4(1.0f, 0.0f, 0.0f, 0.0f);
static const f32x4 _id_y = set_f32x4(0.0f, 1.0f, 0.0f, 0.0f);
static const f32x4 _id_z = set_f32x4(0.0f, 0.0f, 1.0f, 0.0f);
static const f32x4 _id_w = set_f32x4(0.0f, 0.0f, 0.0f, 1.0f);
static const f32x4 _minus1 = set_f32x4(-1.0f, -1.0f, -1.0f, -1.0f);
static const f32x4 _plus1 = set_f32x4(1.0f, 1.0f, 1.0f, 1.0f);
static const f32x4 _zero = set_f32x4(0.0f, 0.0f, 0.0f, 0.0f);
static const i32x4 _sign = set_i32x4(0x80000000, 0x80000000, 0x80000000, 0x80000000);
static const f32x4 _mask_xyz = cast_i32x4_to_f32x4(set_i32x4( 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0 ));

struct NEBULA_ALIGN16 vec3
{
public:
    /// default constructor, NOTE: does NOT setup components!
    vec3() = default;
    /// construct from values
    vec3(scalar x, scalar y, scalar z);
    /// construct from float3
    vec3(float3 f3);
    /// construct from single value
    explicit vec3(scalar v);
    /// copy constructor
    vec3(const vec3& rhs) = default;
    /// construct from SSE 128 byte float array
    vec3(const f32x4& rhs);

    /// assign an vmVector4
    void operator=(const f32x4& rhs);
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
        f32x4 vec;
        float v[3];
    };
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
vec3::vec3(scalar x, scalar y, scalar z)
{
    this->vec = set_f32x4(x, y, z, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vec3::vec3(float3 f3)
{
    this->vec = _mm_setr_ps(f3.x, f3.y, f3.z, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vec3::vec3(scalar v)
{
    this->vec = set_f32x4(v, v, v, 0.0f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vec3::vec3(const f32x4& rhs)
{
    this->vec = set_last_f32x4(rhs, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec3::operator=(const f32x4& rhs)
{
    this->vec = set_last_f32x4(rhs, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
vec3::operator==(const vec3& rhs) const
{
    return all_u32x3(compare_equal_f32x4(this->vec, rhs.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
vec3::operator!=(const vec3 &rhs) const
{
    return !all_u32x3(compare_equal_f32x4(this->vec, rhs.vec));
}

//------------------------------------------------------------------------------
/**
    Load 4 floats from 16-byte-aligned memory.
*/
__forceinline void
vec3::load(const scalar* ptr)
{
    this->vec = load_aligned_f32x3(ptr);
}

//------------------------------------------------------------------------------
/**
    Load 4 floats from unaligned memory.
*/
__forceinline void
vec3::loadu(const scalar* ptr)
{
    this->vec = load_unaligned_f32x3(ptr);
}

//------------------------------------------------------------------------------
/**
    Store to 16-byte-aligned float pointer.
*/
__forceinline void
vec3::store(scalar* ptr) const
{
    store_f32x3(this->vec, ptr);
}

//------------------------------------------------------------------------------
/**
    Store to non-aligned float pointer.
*/
__forceinline void
vec3::storeu(scalar* ptr) const
{
    store_f32x3(this->vec, ptr);
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
    return vec3(flip_sign_f32x4(lhs.vec));
}

//------------------------------------------------------------------------------
/**
    Component-wise multiply with scalar.
*/
__forceinline vec3
operator*(const vec3& lhs, scalar t)
{
    f32x4 temp = _mm_set1_ps(t);
    return mul_f32x4(lhs.vec, temp);
}

//------------------------------------------------------------------------------
/**
    Component-wise multiply with another vector.
*/
__forceinline vec3
operator*(const vec3& lhs, const vec3& rhs)
{
    return mul_f32x4(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
    Component-wise divide by scalar.
*/
__forceinline vec3
operator/(const vec3& lhs, scalar t)
{
    __m128 temp = _mm_set1_ps(t);
    return _mm_div_ps(lhs.vec, temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec3::operator*=(const vec3& rhs)
{
    this->vec = mul_f32x4(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec3::operator/=( const vec3& rhs )
{
    this->vec = div_f32x4(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec3::operator+=(const vec3 &rhs)
{
    this->vec = add_f32x4(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec3::operator-=(const vec3 &rhs)
{
    this->vec = sub_f32x4(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec3::operator*=(scalar s)
{
    f32x4 temp = splat_f32x4(s);
    this->vec = mul_f32x4(this->vec, temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
operator+(const vec3& lhs, const vec3 &rhs)
{
    return add_f32x4(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
operator-(const vec3& lhs, const vec3& rhs)
{
    return sub_f32x4(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vec3::set(scalar x, scalar y, scalar z)
{
    this->vec = set_f32x4(x, y, z, 0);
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
    scalar dot = dot_f32x3(v.vec, v.vec);
    return sqrt(dot);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
lengthsq(const vec3& v)
{
    return dot_f32x3(v.vec, v.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
reciprocal(const vec3& v)
{
    return div_f32x4(_plus1, v.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
reciprocalapprox(const vec3& v)
{
    return rcp_f32x4(v.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
multiply(const vec3& v0, const vec3& v1)
{
    return mul_f32x4(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
multiplyadd( const vec3& v0, const vec3& v1, const vec3& v2 )
{
    return fma_f32x4(v0.vec, v1.vec, v2.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3 
divide(const vec3& v0, const vec3& v1)
{
    return div_f32x4(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
abs(const vec3& v)
{
    return abs_f32x4(v.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
cross(const vec3& v0, const vec3& v1)
{
    f32x4 tmp0, tmp1, tmp2, tmp3, result;
    tmp0 = shuffle_f32x4( v0.vec, v0.vec, 3,0,2,1 );
    tmp1 = shuffle_f32x4( v1.vec, v1.vec, 3,1,0,2 );
    tmp2 = shuffle_f32x4( v0.vec, v0.vec, 3,1,0,2 );
    tmp3 = shuffle_f32x4( v1.vec, v1.vec, 3,0,2,1 );
    result = mul_f32x4( tmp0, tmp1 );
    result = sub_f32x4( result, mul_f32x4( tmp2, tmp3 ) );
    return result;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
dot(const vec3& v0, const vec3& v1)
{
    return dot_f32x3(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
    calculates Result = v0 + f * (v1 - v0) + g * (v2 - v0)
*/
__forceinline vec3
barycentric(const vec3& v0, const vec3 &v1, const vec3 &v2, scalar f, scalar g)
{
    f32x4 R1 = sub_f32x4(v1.vec,v0.vec);
    f32x4 SF = splat_f32x4(f);
    f32x4 R2 = sub_f32x4(v2.vec,v0.vec);
    f32x4 SG = splat_f32x4(g);
    R1 = mul_f32x4(R1,SF);
    R2 = mul_f32x4(R2,SG);
    R1 = add_f32x4(R1,v0.vec);
    R1 = add_f32x4(R1,R2);
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

    f32x4 P0 = splat_f32x4((-s3 + 2.0f * s2 - s) * 0.5f);
    f32x4 P1 = splat_f32x4((3.0f * s3 - 5.0f * s2 + 2.0f) * 0.5f);
    f32x4 P2 = splat_f32x4((-3.0f * s3 + 4.0f * s2 + s) * 0.5f);
    f32x4 P3 = splat_f32x4((s3 - s2) * 0.5f);

    P0 = mul_f32x4(P0, v0.vec);
    P1 = mul_f32x4(P1, v1.vec);
    P2 = mul_f32x4(P2, v2.vec);
    P3 = mul_f32x4(P3, v3.vec);
    P0 = add_f32x4(P0,P1);
    P2 = add_f32x4(P2,P3);
    P0 = add_f32x4(P0,P2);
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

    f32x4 P0 = splat_f32x4(2.0f * s3 - 3.0f * s2 + 1.0f);
    f32x4 T0 = splat_f32x4(s3 - 2.0f * s2 + s);
    f32x4 P1 = splat_f32x4(-2.0f * s3 + 3.0f * s2);
    f32x4 T1 = splat_f32x4(s3 - s2);

    f32x4 vResult = mul_f32x4(P0, v1.vec);
    f32x4 vTemp = mul_f32x4(T0, t1.vec);
    vResult = add_f32x4(vResult,vTemp);
    vTemp = mul_f32x4(P1, v2.vec);
    vResult = add_f32x4(vResult,vTemp);
    vTemp = mul_f32x4(T1, t2.vec);
    vResult = add_f32x4(vResult,vTemp);
    return vResult;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
angle(const vec3& v0, const vec3& v1)
{
    f32x4 l0 = mul_f32x4(v0.vec, v0.vec);
    l0 = add_f32x4(shuffle_f32x4(l0, l0, 0, 0, 0, 0),
           add_f32x4(shuffle_f32x4(l0, l0, 1, 1, 1, 1), shuffle_f32x4(l0, l0, 2, 2, 2, 2)));

    f32x4 l1 = mul_f32x4(v1.vec, v1.vec);
    l1 = add_f32x4(shuffle_f32x4(l1, l1, 0, 0, 0, 0),
           add_f32x4(shuffle_f32x4(l1, l1, 1, 1, 1, 1), shuffle_f32x4(l1, l1, 2, 2, 2, 2)));

    f32x4 l = shuffle_f32x4(l0, l1, 0, 0, 0, 0);
    l = rsqrt_f32x4(l);
    l = mul_first_f32x4(shuffle_f32x4(l, l, 0, 0, 0, 0), shuffle_f32x4(l, l, 1, 1, 1, 1));


    f32x4 dot = mul_f32x4(v0.vec, v1.vec);
    dot = add_f32x4(shuffle_f32x4(dot, dot, 0, 0, 0, 0),
            add_f32x4(shuffle_f32x4(dot, dot, 1, 1, 1, 1),
              add_f32x4(shuffle_f32x4(dot, dot, 2, 2, 2, 2), shuffle_f32x4(dot, dot, 3, 3, 3, 3))));

    dot = mul_first_f32x4(dot, l);

    dot = max_first_f32x4(dot, _minus1);
    dot = min_first_f32x4(dot, _plus1);

    scalar cangle;
    store_f32(dot, &cangle);
    return acos(cangle);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
lerp(const vec3& v0, const vec3& v1, scalar s)
{
    return fma_f32x4(sub_f32x4(v0.vec, v1.vec), splat_f32x4(s), v0.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
maximize(const vec3& v0, const vec3& v1)
{
    return max_f32x4(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
minimize(const vec3& v0, const vec3& v1)
{
    return min_f32x4(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
clamp(const vec3& clamp, const vec3& min, const vec3& max)
{
    f32x4 temp = max_f32x4(min.vec, clamp.vec);
    temp = min_f32x4(temp, max.vec);
    return vec3(temp);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
normalize(const vec3& v)
{
    if (v == vec3(0)) return v;
    
    f32x4 t = div_f32x4(v.vec, splat_f32x4(sqrt(dot_f32x3(v.vec, v.vec))));
    return vec3(t);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
normalizeapprox(const vec3& v)
{
    if (v == vec3(0)) return v;
    f32x4 t = splat_f32x4(1.0f / sqrt(dot_f32x3(v.vec, v.vec)));
    return mul_f32x4(v.vec, set_last_f32x4(t, 0));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
reflect(const vec3& normal, const vec3& incident)
{
    f32x4 res = mul_f32x4(incident.vec, normal.vec);
    res = add_f32x4(shuffle_f32x4(res, res, 0, 0, 0, 0),
        add_f32x4(shuffle_f32x4(res, res, 1, 1, 1, 1), shuffle_f32x4(res, res, 2, 2, 2, 2)));
    res = add_f32x4(res, res);
    res = mul_f32x4(res, normal.vec);
    res = sub_f32x4(incident.vec,res);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
less_any(const vec3& v0, const vec3& v1)
{
    return any_u32x3(compare_less_f32x4(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
less_all(const vec3& v0, const vec3& v1)
{
    return all_u32x3(compare_less_f32x4(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
lessequal_any(const vec3& v0, const vec3& v1)
{
    return any_u32x3(compare_less_equal_f32x4(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
lessequal_all(const vec3& v0, const vec3& v1)
{
    return all_u32x3(compare_less_equal_f32x4(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greater_any(const vec3& v0, const vec3& v1)
{
    return any_u32x3(compare_greater_f32x4(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greater_all(const vec3& v0, const vec3& v1)
{
    return all_u32x3(compare_greater_f32x4(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greaterequal_any(const vec3& v0, const vec3& v1)
{
    return any_u32x3(compare_greater_equal_f32x4(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greaterequal_all(const vec3& v0, const vec3& v1)
{
    return all_u32x3(compare_greater_equal_f32x4(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
equal_any(const vec3& v0, const vec3& v1)
{
    return any_u32x3(compare_equal_f32x4(v0.vec, v1.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
nearequal(const vec3& v0, const vec3& v1, float epsilon)
{
    f32x4 eps = set_f32x4(epsilon, epsilon, epsilon, 0.0f);
    f32x4 delta = sub_f32x4(v0.vec, v1.vec);
    f32x4 temp = splat_f32x4(0);
    temp = sub_f32x4(temp, delta);
    temp = max_f32x4(temp, delta);
    return all_u32x3(compare_less_equal_f32x4(temp, eps));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
nearequal(const vec3& v0, const vec3& v1, const vec3& epsilon)
{
    f32x4 delta = sub_f32x4(v0.vec, v1.vec);
    f32x4 temp = splat_f32x4(0);
    temp = sub_f32x4(temp, delta);
    temp = max_f32x4(temp, delta);
    return all_u32x3(compare_less_equal_f32x4(temp, epsilon.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
less(const vec3& v0, const vec3& v1)
{
    return min_f32x4(convert_u32x4_to_f32x4(compare_less_f32x4(v0.vec, v1.vec)), _plus1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
greater(const vec3& v0, const vec3& v1)
{
    return min_f32x4(convert_u32x4_to_f32x4(compare_greater_f32x4(v0.vec, v1.vec)), _plus1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
equal(const vec3& v0, const vec3& v1)
{
    return min_f32x4(convert_u32x4_to_f32x4(compare_equal_f32x4(v0.vec, v1.vec)), _plus1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
splat(const vec3& v, uint element)
{
    n_assert(element < 3 && element >= 0);

    f32x4 res;
    switch (element)
    {
    case 0:
        res = shuffle_f32x4(v.vec, v.vec, 0, 0, 0, 0);
        break;
    case 1:
        res = shuffle_f32x4(v.vec, v.vec, 1, 1, 1, 1);
        break;
    case 2:
        res = shuffle_f32x4(v.vec, v.vec, 2, 2, 2, 2);
        break;
    }
    return set_last_f32x4(res, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
splat_x(const vec3& v)
{
    f32x4 res = shuffle_f32x4(v.vec, v.vec, 0, 0, 0, 0);
    return set_last_f32x4(res, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
splat_y(const vec3& v)
{
    f32x4 res = shuffle_f32x4(v.vec, v.vec, 1, 1, 1, 1);
    return set_last_f32x4(res, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
splat_z(const vec3& v)
{
    f32x4 res = shuffle_f32x4(v.vec, v.vec, 2, 2, 2, 2);
    return set_last_f32x4(res, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
permute(const vec3& v0, const vec3& v1, unsigned int i0, unsigned int i1, unsigned int i2)
{
    static i32x4 three = _mm_set_epi32(3,3,3,3);

    NEBULA_ALIGN16 unsigned int elem[4] = { i0, i1, i2, 7 };
    i32x4 vControl = _mm_load_si128(reinterpret_cast<const __m128i*>(&elem[0]));

    i32x4 vSelect = _mm_cmpgt_epi32(vControl, three);
    vControl = _mm_and_si128(vControl, three);

    f32x4 shuffled1 = _mm_permutevar_ps(v0.vec, vControl);
    f32x4 shuffled2 = _mm_permutevar_ps(v1.vec, vControl);

    f32x4 masked1 = _mm_andnot_ps(_mm_castsi128_ps(vSelect), shuffled1);
    f32x4 masked2 = _mm_and_ps(_mm_castsi128_ps(vSelect), shuffled2);

    return _mm_or_ps(masked1, masked2);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
select(const vec3& v0, const vec3& v1, const uint i0, const uint i1, const uint i2)
{
    //FIXME this should be converted to something similar as XMVectorSelect
    return permute(v0, v1, i0, i1, i2);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vec3
select(const vec3& v0, const vec3& v1, const vec3& control)
{
    f32x4 v0masked = _mm_andnot_ps(control.vec, v0.vec);
    f32x4 v1masked = _mm_and_ps(v1.vec, control.vec);
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








