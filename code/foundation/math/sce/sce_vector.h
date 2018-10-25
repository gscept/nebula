#pragma once
#ifndef MATH_SCE_VECTOR_H
#define MATH_SCE_VECTOR_H
//------------------------------------------------------------------------------
/**
    @class Math::vector
    
    A vector in homogenous space. A vector describes a direction and length
    in 3d space and always has a w component of 0.0.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "math/sce/sce_float4.h"

//------------------------------------------------------------------------------
namespace Math
{
class vector : public float4
{
public:
    /// default constructor
    vector();
    /// construct from components
    vector(scalar x, scalar y, scalar z);
    /// construct from float4
    vector(const float4& rhs);
    /// copy constructor
    vector(const vector& rhs);
    /// return the null vector
    static vector nullvec();
    /// return the standard up vector (0, 1, 0)
    static vector upvec();
    /// assignment operator
    void operator=(const vector& rhs);
    /// flip sign
    vector operator-() const;
    /// add vector inplace
    void operator+=(const vector& rhs);
    /// subtract vector inplace
    void operator-=(const vector& rhs);
    /// scale vector inplace
    void operator*=(scalar s);
    /// add 2 vectors
    vector operator+(const vector& rhs) const;
    /// subtract 2 vectors
    vector operator-(const vector& rhs) const;
    /// scale vector
    vector operator*(scalar s) const;
    /// equality operator
    bool operator==(const vector& rhs) const;
    /// inequality operator
    bool operator!=(const vector& rhs) const;
    /// set components
    void set(scalar x, scalar y, scalar z);

    friend class point;
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector() :
    float4(0.0f, 0.0f, 0.0f, 0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector(scalar x, scalar y, scalar z) :
    float4(x, y, z, 0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector(const float4& rhs) :
    float4(rhs)
{
    this->W = 0.0f;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
vector::vector(const vector& rhs) :
    float4(rhs.X, rhs.Y, rhs.Z, 0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
vector::nullvec()
{
    return vector(0.0f, 0.0f, 0.0f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
vector::upvec()
{
    return vector(0.0f, 1.0f, 0.0f);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::operator=(const vector& rhs)
{
    this->X = rhs.X;
    this->Y = rhs.Y;
    this->Z = rhs.Z;
    this->W = 0.0;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
vector::operator-() const
{
    return vector(-this->X, -this->Y, -this->Z);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::operator+=(const vector& rhs)
{
    this->X += rhs.X;
    this->Y += rhs.Y;
    this->Z += rhs.Z;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::operator-=(const vector& rhs)
{
    this->X -= rhs.X;
    this->Y -= rhs.Y;
    this->Z -= rhs.Z;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::operator*=(scalar s)
{
    this->X *= s;
    this->Y *= s;
    this->Z *= s;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
vector::operator+(const vector& rhs) const
{
    return vector(this->X + rhs.X, this->Y + rhs.Y, this->Z + rhs.Z);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
vector::operator-(const vector& rhs) const
{
    return vector(this->X - rhs.X, this->Y - rhs.Y, this->Z - rhs.Z);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline vector
vector::operator*(scalar s) const
{
    return vector(this->X * s, this->Y * s, this->Z * s);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
vector::operator==(const vector& rhs) const
{
    return (this->X == rhs.X) && (this->Y == rhs.Y) && (this->Z == rhs.Z);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline bool
vector::operator!=(const vector& rhs) const
{
    return (this->X != rhs.X) || (this->Y != rhs.Y) || (this->Z != rhs.Z);
}    

//------------------------------------------------------------------------------
/**
*/
__forceinline void
vector::set(scalar x, scalar y, scalar z)
{
    float4::set(x, y, z, 0.0f);
}

} // namespace Math
//------------------------------------------------------------------------------
#endif
