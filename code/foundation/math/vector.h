#pragma once
//------------------------------------------------------------------------------
/**
    @struct Math::vector
    
    A vector is a 3D direction in space.

    Represented as a __m128 with the W component being 0 at all times.
    Useful for representing normal vectors and directions.

    @copyright
    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "scalar.h"
#include "vec4.h"
#include "vec3.h"
namespace Math
{

struct vector
{
    /// default constructor
    vector();
    /// construct from values
    vector(scalar x, scalar y, scalar z);
    /// construct from single value
    vector(scalar v);
    /// copy constructor
    vector(const vector& rhs);
    /// construct from vec3
    vector(const vec3& rhs);
    /// construct from vec4
    vector(const vec4& rhs);
    /// construct from SSE 128 byte float array
    vector(const __m128& rhs);

    /// load content from 16-byte-aligned memory
    void load(const scalar* ptr);
    /// load content from unaligned memory
    void loadu(const scalar* ptr);
    /// write content to 16-byte-aligned memory through the write cache
    void store(scalar* ptr) const;
    /// write content to unaligned memory through the write cache
    void storeu(scalar* ptr) const;

    /// assignment operator
    void operator=(const vector& rhs);
    /// assign an vmVector4
    void operator=(const __m128& rhs);
    /// inplace add
    void operator+=(const vector& rhs);
    /// inplace sub
    void operator-=(const vector& rhs);
    /// inplace scalar multiply
    void operator*=(scalar s);
    /// equality operator
    bool operator==(const vector& rhs) const;
    /// inequality operator
    bool operator!=(const vector& rhs) const;
	///
	float operator[](int index) const;
	///
	float& operator[](int index);
    /// convert to vec4
    operator vec4() const;
    /// convert to vec3
    operator vec3() const;

    /// set content
    void set(scalar x, scalar y, scalar z);

    /// create a null vector
    static vector nullvec();
    /// create a 1,1,1 vector
    static vector onevec();
    /// create the up vector
    static vector upvec();

    union
    {
        __m128 vec;
        struct
        {
            float x, y, z;
        };
    };
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector()
{
    this->vec = _mm_set1_ps(0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector(scalar x, scalar y, scalar z)
{
    this->vec = _mm_setr_ps(x, y, z, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector(scalar v)
{
    this->vec = _mm_setr_ps(v, v, v, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector(const vector& rhs)
{
    this->vec = rhs.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector(const vec3& rhs)
{
    this->vec = rhs.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector(const vec4& rhs)
{
    this->vec = rhs.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector(const __m128& rhs)
{
    this->vec = _mm_insert_ps(rhs, _id_w, 0b111000);
}

//------------------------------------------------------------------------------
/**
    Load 4 floats from 16-byte-aligned memory.
*/
__forceinline void
vector::load(const scalar* ptr)
{
    this->vec = _mm_load_ps(ptr);
    this->vec = _mm_and_ps(this->vec, _mask_xyz);
}

//------------------------------------------------------------------------------
/**
    Load 4 floats from unaligned memory.
*/
__forceinline void
vector::loadu(const scalar* ptr)
{
    this->vec = _mm_loadu_ps(ptr);
    this->vec = _mm_and_ps(this->vec, _mask_xyz);
}

//------------------------------------------------------------------------------
/**
    Store to 16-byte-aligned float pointer.
*/
__forceinline void
vector::store(scalar* ptr) const
{
    __m128 v = _mm_shuffle_ps(this->vec, this->vec, _MM_SHUFFLE(2, 2, 2, 2));
    _mm_storel_epi64(reinterpret_cast<__m128i*>(ptr), _mm_castps_si128(this->vec));
    _mm_store_ss(&ptr[2], v);
}

//------------------------------------------------------------------------------
/**
    Store to non-aligned float pointer.
*/
__forceinline void
vector::storeu(scalar* ptr) const
{
    __m128 t1 = _mm_shuffle_ps(this->vec, this->vec, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 t2 = _mm_shuffle_ps(this->vec, this->vec, _MM_SHUFFLE(2, 2, 2, 2));
    _mm_store_ss(&ptr[0], this->vec);
    _mm_store_ss(&ptr[1], t1);
    _mm_store_ss(&ptr[2], t2);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector 
operator-(const vector& rhs)
{
    return vector(_mm_xor_ps(_mm_castsi128_ps(_sign), rhs.vec));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::operator=(const vector& rhs)
{
    this->vec = rhs.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::operator=(const __m128& rhs)
{
    this->vec = _mm_insert_ps(rhs, _id_w, 0b111000);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::operator+=(const vector& rhs)
{
    this->vec = _mm_add_ps(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::operator-=(const vector& rhs)
{
    this->vec = _mm_sub_ps(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::operator*=(scalar s)
{
    this->vec = _mm_mul_ps(this->vec, _mm_set1_ps(s));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
operator+(const vector& lhs, const vector& rhs)
{
    return _mm_add_ps(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
operator-(const vector& lhs, const vector& rhs)
{
    return _mm_sub_ps(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
operator*(const vector& lhs, scalar s)
{
    return _mm_mul_ps(lhs.vec, _mm_set1_ps(s));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
operator*(const vector& lhs, const vector& rhs)
{
    return _mm_mul_ps(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
vector::operator==(const vector& rhs) const
{
    __m128 vTemp = _mm_cmpeq_ps(this->vec, rhs.vec);
    return ((_mm_movemask_ps(vTemp) == 0x0f) != 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
vector::operator!=(const vector& rhs) const
{
    __m128 vTemp = _mm_cmpeq_ps(this->vec, rhs.vec);
    return ((_mm_movemask_ps(vTemp) == 0x0f) == 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float
vector::operator[](int index) const
{
	n_assert(index >= 0 && index < 4);
	return *((&this->x) + index);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline float&
vector::operator[](int index)
{
	n_assert(index >= 0 && index < 4);
	return *((&this->x) + index);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline 
vector::operator vec4() const
{
    return vec4(this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::operator vec3() const
{
    return vec3(this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::set(scalar x, scalar y, scalar z)
{
    this->vec = _mm_setr_ps(x, y, z, 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
normalize(const vector& v)
{
    if (v == vector(0)) return v;
    __m128 t = _mm_sqrt_ps(_mm_dp_ps(v.vec, v.vec, 0xF7));
    t = _mm_or_ps(t, _id_w);
    return _mm_div_ps(v.vec, t);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
normalizeapprox(const vector& v)
{
    if (v == vector(0)) return v;
    __m128 t = _mm_rsqrt_ps(_mm_dp_ps(v.vec, v.vec, 0xF7));
    t = _mm_or_ps(t, _id_w);
    return _mm_mul_ps(v.vec, t);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
dot(const vector& v0, const vector& v1)
{
    return _mm_cvtss_f32(_mm_dp_ps(v0.vec, v1.vec, 0x71));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
cross(const vector& v0, const vector& v1)
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
length(const vector& v)
{
    return _mm_cvtss_f32(_mm_sqrt_ss(_mm_dp_ps(v.vec, v.vec, 0x71)));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
lengthsq(const vector& v)
{
    return _mm_cvtss_f32(_mm_dp_ps(v.vec, v.vec, 0x71));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector 
vector::nullvec()
{
    return vector(0,0,0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
vector::onevec()
{
    return vector(1, 1, 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector 
vector::upvec()
{
    return vector(0, 1, 0);
}

} // namespace Math
