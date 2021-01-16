#pragma once
#ifndef MATH_FLOAT2_H
#define MATH_FLOAT2_H
//------------------------------------------------------------------------------
/**
    @class Math::vec2

    A 2-component float vector class.
    
    (C) 2007 RadonLabs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/scalar.h"

//------------------------------------------------------------------------------
namespace Math
{
struct vec2
{
    /// default constructor, NOTE: does NOT setup components!
    vec2() = default;
    /// construct from single value
    vec2(scalar x);
    /// construct from values
    vec2(scalar x, scalar y);
    /// copy constructor
    vec2(const vec2& rhs) = default;
    /// flip sign
    vec2 operator-() const;
    /// inplace add
    void operator+=(const vec2& rhs);
    /// inplace sub
    void operator-=(const vec2& rhs);
    /// inplace scalar multiply
    void operator*=(scalar s);
    /// add 2 vectors
    vec2 operator+(const vec2& rhs) const;
    /// subtract 2 vectors
    vec2 operator-(const vec2& rhs) const;
    /// multiply with scalar
    vec2 operator*(scalar s) const;
    /// equality operator
    bool operator==(const vec2& rhs) const;
    /// inequality operator
    bool operator!=(const vec2& rhs) const;
    /// set content
    void set(scalar x, scalar y);

    /// load content from memory
    void load(const scalar* ptr);
    /// write content memory
    void store(scalar* ptr) const;

    /// return length of vector
    scalar length() const;
    /// return squared length of vector
    scalar lengthsq() const;
    /// return component-wise absolute
    vec2 abs() const;    
    /// return true if any components are non-zero
    bool any() const;
    /// return true if all components are non-zero
    bool all() const;
    /// component wise multiplication
    static vec2 multiply(const vec2& v0, const vec2& v1);
    /// return vector made up of largest components of 2 vectors
    static vec2 maximize(const vec2& v0, const vec2& v1);
    /// return vector made up of smallest components of 2 vectors
    static vec2 minimize(const vec2& v0, const vec2& v1);
    /// return normalized version of vector
    static vec2 normalize(const vec2& v);
    /// set less-then components to non-zero 
    static vec2 lt(const vec2& v0, const vec2& v1);
    /// set less-or-equal components to non-zero
    static vec2 le(const vec2& v0, const vec2& v1);
    /// set greater-then components to non-zero
    static vec2 gt(const vec2& v0, const vec2& v1);
    /// set greater-or-equal components to non-zero
    static vec2 ge(const vec2& v0, const vec2& v1);

    /// convert to anything
    template<typename T> T as() const;

    scalar x;
    scalar y;
};

//------------------------------------------------------------------------------
/**
*/
inline
vec2::vec2(scalar x) : 
    x(x), y(x)
{
    //  empty
}

//------------------------------------------------------------------------------
/**
*/
inline 
vec2::vec2(scalar x, scalar y) :
    x(x), y(y)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline bool
vec2::operator==(const vec2& rhs) const
{
    return (this->x == rhs.x) && (this->y == rhs.y);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
vec2::operator!=(const vec2& rhs) const
{
    return (this->x != rhs.x) || (this->y != rhs.y);
}

//------------------------------------------------------------------------------
/**
*/
inline vec2
vec2::operator-() const
{
    return vec2(-this->x, -this->y);
}

//------------------------------------------------------------------------------
/**
*/
inline vec2
vec2::operator*(scalar t) const
{
    return vec2(this->x * t, this->y * t);
}

//------------------------------------------------------------------------------
/**
*/
inline void
vec2::operator+=(const vec2& rhs)
{
    this->x += rhs.x;
    this->y += rhs.y;
}

//------------------------------------------------------------------------------
/**
*/
inline void
vec2::operator-=(const vec2& rhs)
{
    this->x -= rhs.x;
    this->y -= rhs.y;
}

//------------------------------------------------------------------------------
/**
*/
inline void
vec2::operator*=(scalar s)
{
    this->x *= s;
    this->y *= s;
}

//------------------------------------------------------------------------------
/**
*/
inline vec2
vec2::operator+(const vec2& rhs) const
{
    return vec2(this->x + rhs.x, this->y + rhs.y);
}

//------------------------------------------------------------------------------
/**
*/
inline vec2
vec2::operator-(const vec2& rhs) const
{
    return vec2(this->x - rhs.x, this->y - rhs.y);
}

//------------------------------------------------------------------------------
/**
*/
inline void
vec2::set(scalar x, scalar y)
{
    this->x = x;
    this->y = y;
}

//------------------------------------------------------------------------------
/**
*/
inline
void vec2::load(const scalar* ptr)
{
    this->x = ptr[0];
    this->y = ptr[1];
}

//------------------------------------------------------------------------------
/**
*/
inline
void vec2::store(scalar* ptr) const
{
    ptr[0] = this->x;
    ptr[1] = this->y;
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
vec2::length() const
{
    return Math::sqrt(this->x * this->x + this->y * this->y);
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
vec2::lengthsq() const
{
    return this->x * this->x + this->y * this->y;
}

//------------------------------------------------------------------------------
/**
*/
inline vec2
vec2::abs() const
{
    return vec2(Math::abs(this->x), Math::abs(this->y));
}

//------------------------------------------------------------------------------
/**
*/
inline bool
vec2::any() const
{
    return (this->x != 0.0f) || (this->y != 0.0f);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
vec2::all() const
{
    return (this->x != 0.0f) && (this->y != 0.0f);
}

//------------------------------------------------------------------------------
/**
*/
inline vec2
vec2::lt(const vec2& v0, const vec2& v1)
{
    vec2 res;
    res.x = (v0.x < v1.x) ? 1.0f : 0.0f;
    res.y = (v0.y < v1.y) ? 1.0f : 0.0f;
    return res;
}

//------------------------------------------------------------------------------
/**
*/
inline vec2
vec2::le(const vec2& v0, const vec2& v1)
{
    vec2 res;
    res.x = (v0.x <= v1.x) ? 1.0f : 0.0f;
    res.y = (v0.y <= v1.y) ? 1.0f : 0.0f;
    return res;
}

//------------------------------------------------------------------------------
/**
*/
inline vec2
vec2::gt(const vec2& v0, const vec2& v1)
{
    vec2 res;
    res.x = (v0.x > v1.x) ? 1.0f : 0.0f;
    res.y = (v0.y > v1.y) ? 1.0f : 0.0f;
    return res;
}

//------------------------------------------------------------------------------
/**
*/
inline vec2
vec2::ge(const vec2& v0, const vec2& v1)
{
    vec2 res;
    res.x = (v0.x >= v1.x) ? 1.0f : 0.0f;
    res.y = (v0.y >= v1.y) ? 1.0f : 0.0f;
    return res;
}

//------------------------------------------------------------------------------
/**
*/
inline vec2
vec2::normalize(const vec2& v)
{
    scalar l = v.length();
    if (l > 0.0f)
    {
        return vec2(v.x / l, v.y / l);
    }
    else
    {
        return vec2(1.0f, 0.0f);
    }
}


//------------------------------------------------------------------------------
/**
*/
inline vec2 
vec2::multiply( const vec2& v0, const vec2& v1 )
{
    return vec2(v0.x * v1.x, v0.y * v1.y);
}

//------------------------------------------------------------------------------
/**
*/
inline vec2
vec2::maximize(const vec2& v0, const vec2& v1)
{
    return vec2(Math::max(v0.x, v1.x), Math::max(v0.y, v1.y));
}

//------------------------------------------------------------------------------
/**
*/
inline vec2
vec2::minimize(const vec2& v0, const vec2& v1)
{
    return vec2(Math::min(v0.x, v1.x), Math::min(v0.y, v1.y));
}

} // namespace Math
//------------------------------------------------------------------------------
#endif
