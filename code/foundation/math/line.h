#pragma once
#ifndef MATH_LINE_H
#define MATH_LINE_H
//------------------------------------------------------------------------------
/**
    @class Math::line

    A line in 3d space.

    (C) 2004 RadonLabs GmbH
	(C) 2013-2020 Individual contributors, see AUTHORS file
*/

#include "math2/vec3.h"
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
    line(const vec3& startPoint, const vec3& endPoint);
    /// copy constructor
    line(const line& rhs);
    /// set start and end point
    void set(const vec3& startPoint, const vec3& endPoint);
    /// set start point and direction
    void set_point_dir(const vec3& startPoint, const vec3& direction);
    /// get start point
    const vec3& start() const;
    /// get end point
    vec3 end() const;
    /// get vector
    const vec3& vec() const;
    /// get length
    scalar length() const;
    /// get squared length
    scalar lengthsq() const;
    /// minimal distance of point to line
    scalar distance(const vec3& p) const;
    /// intersect with line
    bool intersect(const line& l, vec3& pa, vec3& pb) const;
    /// calculates shortest distance between lines
    scalar distance(const line& l, vec3& pa, vec3& pb) const;
    /// return t of the closest point on the line
    scalar closestpoint(const vec3& p) const;
    /// return p = b + m*t
    vec3 pointat(scalar t) const;

    vec3 b;
    vec3 m;
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
line::line(const vec3& startPoint, const vec3& endPoint) :
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
line::set(const vec3& startPoint, const vec3& endPoint)
{
    this->b = startPoint;
    this->m = endPoint - startPoint;
}

//------------------------------------------------------------------------------
/**
*/
inline void
line::set_point_dir(const vec3& startPoint, const vec3& dir)
{
    this->b = startPoint;
    this->m = dir;
}

//------------------------------------------------------------------------------
/**
*/
inline const vec3&
line::start() const
{
    return this->b;
}

//------------------------------------------------------------------------------
/**
*/
inline vec3
line::end() const
{
    return this->b + this->m;
}

//------------------------------------------------------------------------------
/**
*/
inline const vec3&
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
    return Math::length(this->m);
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
line::lengthsq() const
{
    return Math::lengthsq(this->m);
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
line::closestpoint(const vec3& p) const
{
    vec3 diff(p - this->b);
    scalar l = dot(this->m, this->m);
    if (l > 0.0f)
    {
        scalar t = dot(this->m, diff) / l;
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
line::distance(const vec3& p) const
{
    vec3 diff(p - this->b);
    scalar l = dot(this->m, this->m);
    if (l > 0.0f) 
    {
        scalar t = dot(this->m, diff) / l;
        diff = diff - this->m * t;
        return Math::length(diff);
    } 
    else 
    {
        // line is really a point...
        vec3 v(p - this->b);
        return Math::length(v);
    }
}

//------------------------------------------------------------------------------
/**
    Returns p = b + m * t, given t. Note that the point is not on the line
    if 0.0 > t > 1.0
*/
inline vec3
line::pointat(scalar t) const
{
    return this->b + this->m * t;
}

} // namespace Math
//------------------------------------------------------------------------------
#endif
