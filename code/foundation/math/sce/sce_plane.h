#pragma once
#ifndef MATH_SCE_PLANE_H
#define MATH_SCE_PLANE_H
//------------------------------------------------------------------------------
/**
    @class Math::plane
    
    A plane class on top of SCE math functions.
    
    (C) 2007 Radon Labs GmbH
*/
#include "core/types.h"
#include "math/sce/sce_scalar.h"
#include "math/sce/sce_float4.h"

//------------------------------------------------------------------------------
namespace Math
{
class matrix44;
    
class plane
{
public:
    /// default constructor, NOTE: does NOT setup componenets!
    plane();
    /// construct from components
    plane(scalar a, scalar b, scalar c, scalar d);
    /// construct from points
    plane(const float4& p0, const float4& p1, const float4& p2);
    /// construct from point and normal
    plane(const float4& p, const float4& n);
    /// copy constructor
    plane(const plane& rhs);

    /// set componenets
    void set(scalar a, scalar b, scalar c, scalar d);
    /// read/write access to A component
    scalar& a();
    /// read/write access to B component
    scalar& b();
    /// read/write access to C component
    scalar& c();
    /// read/write access to D component
    scalar& d();
    /// read-only access to A component
    scalar a() const;
    /// read-only access to B component
    scalar b() const;
    /// read-only access to C component
    scalar c() const;
    /// read-only access to D component
    scalar d() const;

    /// compute dot product of plane and vector
    scalar dot(const float4& v) const;
    /// find intersection with line
    bool intersectline(const float4& startPoint, const float4& endPoint, float4& outIntersectPoint);

    /// normalize plane components a,b,c
    static plane normalize(const plane& p);
    /// transform plane by inverse transpose of transform
    static plane transform(const plane& p, const matrix44& m);

private:
    scalar A;
    scalar B;
    scalar C;
    scalar D;
};

//------------------------------------------------------------------------------
/**
*/
inline
plane::plane()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
plane::plane(scalar a, scalar b, scalar c, scalar d) :
    A(a), B(b), C(c), D(d)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline
plane::plane(const float4& p0, const float4& p1, const float4& p2)
{
    // D3DXPlaneFromPoints((D3DXPLANE*)this, (CONST D3DXVECTOR3*)&p0, (CONST D3DXVECTOR3*)&p1, (CONST D3DXVECTOR3*)&p2);
}

//------------------------------------------------------------------------------
/**
*/
inline
plane::plane(const float4& p0, const float4& n)
{
    // D3DXPlaneFromPointNormal((D3DXPLANE*)this, (CONST D3DXVECTOR3*)&p0, (CONST D3DXVECTOR3*)&n);
}

//------------------------------------------------------------------------------
/**
*/
inline
plane::plane(const plane& rhs) :
    A(rhs.A), B(rhs.B), C(rhs.C), D(rhs.D)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline void
plane::set(scalar a, scalar b, scalar c, scalar d)
{
    this->A = a;
    this->B = b;
    this->C = c;
    this->D = d;
}

//------------------------------------------------------------------------------
/**
*/
inline scalar&
plane::a()
{
    return this->A;
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
plane::a() const
{
    return this->A;
}

//------------------------------------------------------------------------------
/**
*/
inline scalar&
plane::b()
{
    return this->B;
}

//------------------------------------------------------------------------------
/**
*/ 
inline scalar
plane::b() const
{
    return this->B;
}

//------------------------------------------------------------------------------
/**
*/
inline scalar&
plane::c()
{
    return this->C;
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
plane::c() const
{
    return this->C;
}

//------------------------------------------------------------------------------
/**
*/
inline scalar&
plane::d()
{
    return this->D;
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
plane::d() const
{
    return this->D;
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
plane::dot(const float4& v) const
{
    // return D3DXPlaneDot((CONST D3DXPLANE*)this, (CONST D3DXVECTOR4*)&v);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
plane::intersectline(const float4& startPoint, const float4& endPoint, float4& outIntersectPoint)
{
    outIntersectPoint.set(0.0f, 0.0f, 0.0f, 1.0f);
    /*if (0 != D3DXPlaneIntersectLine((D3DXVECTOR3*)&outIntersectPoint, (CONST D3DXPLANE*)this, (CONST D3DXVECTOR3*)&startPoint, (CONST D3DXVECTOR3*)&endPoint))
    {
        return true;
    }
    else
    {
        return false;
    }
    */
    return false;
}

//------------------------------------------------------------------------------
/**
*/
inline plane
plane::normalize(const plane& p)
{
    plane res;
    // D3DXPlaneNormalize((D3DXPLANE*)&res, (CONST D3DXPLANE*)&p);
    return res;
}

} // namespace Math
//------------------------------------------------------------------------------
#endif
