#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::sphere

    A 3-dimensional sphere.

    (C) 2004 RadonLabs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "math2/vec3.h"
#include "math2/mat4.h"
#include "math/bbox.h"
#include "math/rectangle.h"
#include "math/clipstatus.h"
#include "math/line.h"

//------------------------------------------------------------------------------
namespace Math
{
class sphere 
{
public:
    /// default constructor
    sphere();
    /// pos/radius constructor
    sphere(const vec3& _p, scalar _r);
    /// x,y,z,r constructor
    sphere(scalar _x, scalar _y, scalar _z, scalar _r);
    /// copy constructor
    sphere(const sphere& rhs);
    /// set position and radius
    void set(const vec3& _p, scalar _r);
    /// set x,y,z, radius
    void set(scalar _x, scalar _y, scalar _z, scalar _r);
    /// return true if box is completely inside sphere
    bool inside(const bbox& box) const;
    /// check if 2 spheres overlap
    bool intersects(const sphere& s) const;
    /// check if sphere intersects box
    bool intersects(const bbox& box) const;
    /// check if sphere intersects ray
    bool intersects_ray(const line& l) const;
    /// check if 2 moving sphere have contact
    bool intersect_sweep(const vec3& va, const sphere& sb, const vec3& vb, scalar& u0, scalar& u1) const;
    /// project sphere to screen rectangle (right handed coordinate system)
    rectangle<scalar> project_screen_rh(const mat4& modelView, const mat4& projection, scalar nearZ) const;
    /// get clip status of box against sphere
    ClipStatus::Type clipstatus(const bbox& box) const;
    /// generate a random point on a unit sphere
    static vec3 random_point_on_unit_sphere();
        
    vec3 p;
    scalar r;
};

//------------------------------------------------------------------------------
/**
*/
inline
sphere::sphere() :
    r(1.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
sphere::sphere(const vec3& _p, scalar _r) :
    p(_p),
    r(_r)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
sphere::sphere(scalar _x, scalar _y, scalar _z, scalar _r) :
    p(_x, _y, _z),
    r(_r)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
sphere::sphere(const sphere& rhs) :
    p(rhs.p),
    r(rhs.r)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
sphere::set(const vec3& _p, scalar _r)
{
    this->p = _p;
    this->r = _r;
}

//------------------------------------------------------------------------------
/**
*/
inline void
sphere::set(scalar _x, scalar _y, scalar _z, scalar _r)
{
    this->p.set(_x, _y, _z);
    this->r = _r;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
sphere::intersects(const sphere& s) const 
{
    vec3 d(s.p - p);
    scalar rsum = s.r + r;
    return (lengthsq(d) <= (rsum * rsum));
}

//------------------------------------------------------------------------------
/**
    Return true if the bounding box is inside the sphere.
*/
inline bool
sphere::inside(const bbox& box) const
{
    vec3 v(this->r, this->r, this->r);
    vec3 pmin(this->p - v);
    vec3 pmax(this->p + v);
    bool lt = less_all(box.pmin, pmin);
    bool ge = greaterequal_all(box.pmax, pmax);
    return lt && ge;
}

//------------------------------------------------------------------------------
/**
    Get the clip status of a box against this sphere. Inside means: the
    box is completely inside the sphere.
*/
inline
ClipStatus::Type
sphere::clipstatus(const bbox& box) const
{
    if (this->inside(box)) return ClipStatus::Inside;
    else if (this->intersects(box)) return ClipStatus::Clipped;
    else return ClipStatus::Outside;
}

} // namespace Math
//------------------------------------------------------------------------------