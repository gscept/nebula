#pragma once
#ifndef MATH_LINE_H
#define MATH_LINE_H
//------------------------------------------------------------------------------
/**
    @class Math::line

    A line in 3d space.

    (C) 2004 RadonLabs GmbH
	(C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "math/point.h"
#include "math/vector.h"
#include "math/scalar.h"

//------------------------------------------------------------------------------
namespace Math
{
class line
{
public:
    /// default constructor
    line();
    /// component constructor
    line(const point& startPoint, const point& endPoint);
    /// copy constructor
    line(const line& rhs);
    /// set start and end point
    void set(const point& startPoint, const point& endPoint);
    /// get start point
    const point& start() const;
    /// get end point
    point end() const;
    /// get vector
    const vector& vec() const;
    /// get length
    scalar length() const;
    /// get squared length
    scalar lengthsq() const;
    /// minimal distance of point to line
    scalar distance(const point& p) const;
    /// intersect with line
    bool intersect(const line& l, point& pa, point& pb) const;
    /// calculates shortest distance between lines
    scalar distance(const line& l, point& pa, point& pb) const;
    /// return t of the closest point on the line
    scalar closestpoint(const point& p) const;
    /// return p = b + m*t
    point pointat(scalar t) const;

    point b;
    vector m;
};

//------------------------------------------------------------------------------
/**
*/
inline
line::line()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
line::line(const point& startPoint, const point& endPoint) :
    b(startPoint),
    m(endPoint - startPoint)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
line::line(const line& rhs) :
    b(rhs.b),
    m(rhs.m)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
line::set(const point& startPoint, const point& endPoint)
{
    this->b = startPoint;
    this->m = endPoint - startPoint;
}

//------------------------------------------------------------------------------
/**
*/
inline const point&
line::start() const
{
    return this->b;
}

//------------------------------------------------------------------------------
/**
*/
inline point
line::end() const
{
    return this->b + this->m;
}

//------------------------------------------------------------------------------
/**
*/
inline const vector&
line::vec() const
{
    return this->m;
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
line::length() const
{
    return this->m.length();
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
line::lengthsq() const
{
    return this->m.lengthsq();
}

//------------------------------------------------------------------------------
/**
    Returns a point on the line which is closest to a another point in space.
    This just returns the parameter t on where the point is located. If t is
    between 0 and 1, the point is on the line, otherwise not. To get the
    actual 3d point p: 

    p = m + b*t
*/
inline scalar
line::closestpoint(const point& p) const
{
    vector diff(p - this->b);
    scalar l = float4::dot3(this->m, this->m);
    if (l > 0.0f)
    {
        scalar t = float4::dot3(this->m, diff) / l;
        return t;
    }
    else
    {
        return 0.0f;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
line::distance(const point& p) const
{
    vector diff(p - this->b);
    scalar l = float4::dot3(this->m, this->m);
    if (l > 0.0f) 
    {
        scalar t = float4::dot3(this->m, diff) / l;
        diff = diff - this->m * t;
        return diff.length();
    } 
    else 
    {
        // line is really a point...
        vector v(p - this->b);
        return v.length();
    }
}

//------------------------------------------------------------------------------
/**
    Returns p = b + m * t, given t. Note that the point is not on the line
    if 0.0 > t > 1.0
*/
inline point
line::pointat(scalar t) const
{
    return this->b + this->m * t;
}

} // namespace Math
//------------------------------------------------------------------------------
#endif
