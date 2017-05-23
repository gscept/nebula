#pragma once
#ifndef MATH_POLAR_H
#define MATH_POLAR_H
//------------------------------------------------------------------------------
/**
    @class Math::polar

    A polar coordinate inline class, consisting of 2 angles theta (latitude)
    and rho (longitude). Also offers conversion between cartesian and 
    polar space.

    Allowed range for theta is 0..180 degree (in rad!) and for rho 0..360 degree
    (in rad).

    (C) 2004 RadonLabs GmbH
	(C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "math/scalar.h"
#include "math/vector.h"
#include "math/float2.h"

//------------------------------------------------------------------------------
namespace Math
{
class polar
{
public:
    /// the default constructor
    polar();
    /// constructor, theta and rho args
    polar(scalar t, scalar r);
    /// constructor, normalized cartesian vector as arg
    polar(const vector& v);
    /// the copy constructor
    polar(const polar& src);
    /// the assignment operator
    void operator=(const polar& rhs);
    /// convert to normalized cartesian coords 
    vector get_cartesian() const;
    /// set to polar object
    void set(const polar& p);
    /// set to theta and rho
    void set(scalar t, scalar r);
    /// set to cartesian 
    void set(const vector& v);

    scalar theta;
    scalar rho;
};

//------------------------------------------------------------------------------
/**
*/
inline
polar::polar() :
    theta(0.0f),
    rho(0.0f)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
polar::polar(scalar t, scalar r) :
    theta(t),
    rho(r)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
polar::polar(const vector& v)
{
    this->set(v);
}

//------------------------------------------------------------------------------
/**
*/
inline
polar::polar(const polar& src) :
    theta(src.theta),
    rho(src.rho)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
polar::operator=(const polar& rhs)
{
    this->theta = rhs.theta;
    this->rho = rhs.rho;
}

//------------------------------------------------------------------------------
/**
*/
inline void
polar::set(const polar& p)
{
    this->theta = p.theta;
    this->rho = p.rho;
}

//------------------------------------------------------------------------------
/**
*/
inline void
polar::set(scalar t, scalar r)
{
    this->theta = t;
    this->rho = r;
}

//------------------------------------------------------------------------------
/**
    Convert cartesian to polar.
*/
inline void
polar::set(const vector& vec)
{
    vector normVec3d = vector::normalize(vec);
    this->theta = n_acos(normVec3d.y());

    // build a normalized 2d vector of the xz component
    float2 normVec2d(normVec3d.x(), normVec3d.z());
    if (normVec2d.length() > TINY)
    {
        normVec2d = float2::normalize(normVec2d);
    }
    else
    {
        normVec2d.set(1.0f, 0.0f);
    }

    // adjust dRho based on the quadrant we are in
    if ((normVec2d.x() >= 0.0f) && (normVec2d.y() >= 0.0f))
    {
        // quadrant 1
        this->rho = n_asin(normVec2d.x());
    }
    else if ((normVec2d.x() < 0.0f) && (normVec2d.y() >= 0.0f))
    {
        // quadrant 2 
        this->rho = n_asin(normVec2d.y()) + n_deg2rad(270.0f);
    }
    else if ((normVec2d.x() < 0.0f) && (normVec2d.y() < 0.0f))
    {
        // quadrant 3
        this->rho = n_asin(-normVec2d.x()) + n_deg2rad(180.0f);
    }
    else
    {
        // quadrant 4
        this->rho = n_asin(-normVec2d.y()) + n_deg2rad(90.0f);
    }
}
    
//------------------------------------------------------------------------------
/**
    Convert polar to cartesian.
*/
inline vector
polar::get_cartesian() const
{
    scalar sinTheta = n_sin(this->theta);
    scalar cosTheta = n_cos(this->theta);
    scalar sinRho   = n_sin(this->rho);
    scalar cosRho   = n_cos(this->rho);
    vector v(sinTheta * sinRho, cosTheta, sinTheta * cosRho);
    return v;
}

} // namespace Math
//------------------------------------------------------------------------------
#endif
