#pragma once
//------------------------------------------------------------------------------
/**
    A point is a 4d vector with a fixed W coordinate of 1, which allows it to be 
    transformed. A point implements a limited set of operators and functions, which
    distinguishes it from vec4, vec3 and vector.

    (C) 2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "vector.h"
namespace Math
{

struct point
{

    /// default constructor
    point();
    /// construct from values
    point(scalar x, scalar y, scalar z);
    /// construct from single value
    point(scalar v);
    /// copy constructor
    point(const point& rhs);
    /// construct from vec3
    point(const vec3& rhs);
    /// construct from vec4
    point(const vec4& rhs);
    /// construct from SSE 128 byte float array
    point(const __m128& rhs);

    /// write content to 16-byte-aligned memory through the write cache
    void store(scalar* ptr) const;
    /// write content to unaligned memory through the write cache
    void storeu(scalar* ptr) const;

    /// assignment operator
    void operator=(const point& rhs);
    /// assign an vmVector4
    void operator=(const __m128& rhs);
    /// inplace add
    void operator+=(const vector& rhs);
    /// inplace sub
    void operator-=(const vector& rhs);
    /// equality operator
    bool operator==(const point& rhs) const;
    /// inequality operator
    bool operator!=(const point& rhs) const;
    /// convert to vec4
    operator vec4() const;

    /// set content
    void set(scalar x, scalar y, scalar z);

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
point::point()
{
    this->vec = _mm_setr_ps(0, 0, 0, 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
point::point(scalar x, scalar y, scalar z)
{
    this->vec = _mm_setr_ps(x, y, z, 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
point::point(scalar v)
{
    this->vec = _mm_setr_ps(v, v, v, 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
point::point(const point& rhs)
{
    this->vec = rhs.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
point::point(const vec3& rhs)
{
    // mask out xyz and set w to 1
#if NEBULA_DEBUG
    n_assert(rhs.__w == 0.0f);
#endif
    this->vec = _mm_or_ps(rhs.vec, _id_w);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
point::point(const vec4& rhs)
{
    // mask out xyz and set w to 1
    this->vec = _mm_or_ps(_mm_and_ps(rhs.vec, _mask_xyz), _id_w);;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
point::point(const __m128& rhs)
{
    this->vec = rhs;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
point::operator=(const point& rhs)
{
    this->vec = rhs.vec;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
point::operator=(const __m128& rhs)
{
    this->vec = rhs;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
point::operator+=(const vector& rhs)
{
    this->vec = _mm_add_ps(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
point::operator-=(const vector& rhs)
{
    this->vec = _mm_sub_ps(this->vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
    Store to 16-byte-aligned float pointer.
*/
__forceinline void
point::store(scalar* ptr) const
{
    _mm_store_ps(ptr, this->vec);
}

//------------------------------------------------------------------------------
/**
    Store to non-aligned float pointer.
*/
__forceinline void
point::storeu(scalar* ptr) const
{
    _mm_storeu_ps(ptr, this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline point
operator+(const point& lhs, const vector& rhs)
{
    return _mm_add_ps(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline point
operator-(const point& lhs, const vector& rhs)
{
    return _mm_sub_ps(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
operator-(const point& lhs, const point& rhs)
{
    return _mm_sub_ps(lhs.vec, rhs.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
point::operator==(const point& rhs) const
{
    __m128 vTemp = _mm_cmpeq_ps(this->vec, rhs.vec);
    return ((_mm_movemask_ps(vTemp) == 0x0f) != 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
point::operator!=(const point& rhs) const
{
    __m128 vTemp = _mm_cmpeq_ps(this->vec, rhs.vec);
    return ((_mm_movemask_ps(vTemp) == 0x0f) == 0);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
point::operator vec4() const
{
    return vec4(this->vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
point::set(scalar x, scalar y, scalar z)
{
    this->vec = _mm_setr_ps(x, y, z, 1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
dot(const point& p0, const point& p1)
{
    return _mm_cvtss_f32(_mm_dp_ps(p0.vec, p1.vec, 0x71));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
dot(const vector& v, const point& p)
{
    return _mm_cvtss_f32(_mm_dp_ps(v.vec, p.vec, 0x71));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
dot(const point& p, const vector& v)
{
    return _mm_cvtss_f32(_mm_dp_ps(p.vec, v.vec, 0x71));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline point
maximize(const point& v0, const point& v1)
{
    return _mm_max_ps(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline point
minimize(const point& v0, const point& v1)
{
    return _mm_min_ps(v0.vec, v1.vec);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
less_any(const point& v0, const point& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
less_all(const point& v0, const point& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res == 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
lessequal_any(const point& v0, const point& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
lessequal_all(const point& v0, const point& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res == 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greater_any(const point& v0, const point& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greater_all(const point& v0, const point& v1)
{
    __m128 vTemp = _mm_cmpgt_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res == 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greaterequal_any(const point& v0, const point& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
greaterequal_all(const point& v0, const point& v1)
{
    __m128 vTemp = _mm_cmpge_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res == 0x7;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
equal_any(const point& v0, const point& v1)
{
    __m128 vTemp = _mm_cmpeq_ps(v0.vec, v1.vec);
    int res = _mm_movemask_ps(vTemp) & 7;
    return res != 0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
nearequal(const point& v0, const point& v1, const point& epsilon)
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
__forceinline point
less(const point& v0, const point& v1)
{
    return _mm_min_ps(_mm_cmplt_ps(v0.vec, v1.vec), _plus1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline point
greater(const point& v0, const point& v1)
{
    return _mm_min_ps(_mm_cmpgt_ps(v0.vec, v1.vec), _plus1);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline point
equal(const point& v0, const point& v1)
{
    return _mm_min_ps(_mm_cmpeq_ps(v0.vec, v1.vec), _plus1);
}

} // namespace Math
