#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::plane

    A plane class on top of VectorMath functions.

    (C) 2007 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "core/types.h"
#include "math/scalar.h"
#include "math/float4.h"
#include "math/line.h"
#include "math/clipstatus.h"

//------------------------------------------------------------------------------
namespace Math
{
class matrix44;
class plane;

#if (defined(_XM_VMX128_INTRINSICS_) && !defined(_XM_NO_INTRINSICS_))
typedef const plane __PlaneArg;
#else
// win32 VC++ does not support passing aligned objects as value so far
// here is a bug-report at microsoft saying so:
// http://connect.microsoft.com/VisualStudio/feedback/ViewFeedback.aspx?FeedbackID=334581
typedef const plane& __PlaneArg;
#endif


class NEBULA_ALIGN16 plane
{
public:
    /// default constructor, NOTE: does NOT setup componenets!
    plane();
    /// !!!! copy constructor forbidden, otherwise passing plane's to a function
    /// !!!! via Registers doesnt work
    //plane(const plane& rhs);
    /// construct from components
    plane(scalar a, scalar b, scalar c, scalar d);
    /// construct from points
    plane(const float4& p0, const float4& p1, const float4& p2);
    /// construct from point and normal
    plane(const float4& p, const float4& n);
    /// construct from __m128
    plane(const __m128 & rhs);
	/// construct from float4
	plane(const float4 & rhs);

    /// setup from points
    void setup_from_points(const float4& p0, const float4& p1, const float4& p2);
    /// setup from point and normal
    void setup_from_point_and_normal(const float4& p, const float4& n);
    /// set componenets
    void set(scalar a, scalar b, scalar c, scalar d);
    /// set the x component
    void set_a(scalar a);
    /// set the y component
    void set_b(scalar b);
    /// set the z component
    void set_c(scalar c);
    /// set the w component
    void set_d(scalar d);

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

	/// get the plane normal
	float4 get_normal() const;
	/// get the plane point
	float4 get_point() const;

    /// compute dot product of plane and vector
    scalar dot(const float4& v) const;
    /// find intersection with line
    bool intersectline(const float4& startPoint, const float4& endPoint, float4& outIntersectPoint) const;
	/// find intersection with plane
	bool intersectplane(const plane& p2, line& outLine) const;
    /// clip line against this plane
    ClipStatus::Type clip(const line& l, line& outClippedLine) const;
    /// normalize plane components a,b,c
    static plane normalize(const plane& p);

    /// transform plane by inverse transpose of transform
    static
    //__declspec(deprecated)
     plane transform(const plane& p, const matrix44& m);

private:
    friend class matrix44;

    float4 vec;
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
plane::plane(scalar a, scalar b, scalar c, scalar d)
{
    this->vec = float4(a, b, c, d);
}

//------------------------------------------------------------------------------
/**
*/
inline
plane::plane(const float4& p0, const float4& p1, const float4& p2)
{
    this->setup_from_points(p0, p1, p2);
}

//------------------------------------------------------------------------------
/**
*/
inline
plane::plane(const float4& p0, const float4& n)
{
    this->setup_from_point_and_normal(p0, n);
}

//------------------------------------------------------------------------------
/**
*/
inline
plane::plane(const __m128 & rhs) :
    vec(rhs)
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
inline
plane::plane(const float4 & rhs) :
vec(rhs)
{
	// empty
}
//------------------------------------------------------------------------------
/**
*/
inline void
plane::setup_from_points(const float4& p0, const float4& p1, const float4& p2)
{
	float4 v21 = p0 - p1;
	float4 v31 = p0 - p2;

	float4 cr = float4::cross3(v21,v31);
	cr = float4::normalize(cr);
	float d = float4::dot3(cr,p0);

	this->vec = cr;
	this->vec.set_w(-d);
}

//------------------------------------------------------------------------------
/**
*/
inline void
plane::setup_from_point_and_normal(const float4& p, const float4& n)
{

	float d = float4::dot3(p,n);
	this->vec = n;
	this->vec.set_w(-d);
}

//------------------------------------------------------------------------------
/**
*/
inline void
plane::set(scalar a, scalar b, scalar c, scalar d)
{
    this->vec = float4(a, b, c, d);
}

//------------------------------------------------------------------------------
/**
*/
inline scalar&
plane::a()
{
	return this->vec.x();
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
plane::a() const
{
    return this->vec.x();
}

//------------------------------------------------------------------------------
/**
*/
inline scalar&
plane::b()
{
	return this->vec.y();
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
plane::b() const
{
    return this->vec.y();
}

//------------------------------------------------------------------------------
/**
*/
inline scalar&
plane::c()
{
	return this->vec.z();
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
plane::c() const
{
    return this->vec.z();
}

//------------------------------------------------------------------------------
/**
*/
inline scalar&
plane::d()
{
	return this->vec.w();
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
plane::d() const
{
	return this->vec.w();
}

//------------------------------------------------------------------------------
/**
*/
inline scalar
plane::dot(const float4& v) const
{
	return float4::dot(this->vec,v.vec);
}

//------------------------------------------------------------------------------
/**
*/
inline bool
plane::intersectline(const float4& startPoint, const float4& endPoint, float4& outIntersectPoint) const
{

	scalar v1 = float4::dot3(this->vec,startPoint);
	scalar v2 = float4::dot3(this->vec,endPoint);
	scalar d = (v1-v2);
	if( n_abs(d) < N_TINY)
	{
		return false;
	}

	d = 1.0f/d;

	scalar pd = this->dot(startPoint);
	pd *= d;

	float4 p = (endPoint - startPoint) * pd;

	p += startPoint;
	outIntersectPoint = p;

	return true;
}

//------------------------------------------------------------------------------
/**
*/
inline bool
plane::intersectplane(const plane& p2, line& outLine) const
{
	vector n0 = this->get_normal();
	vector n1 = p2.get_normal();
	float n00 = vector::dot3(n0, n0);
	float n01 = vector::dot3(n0, n1);
	float n11 = vector::dot3(n1, n1);
	float det = n00 * n11 - n01 * n01;
	const float tol = N_TINY;
	if (fabs(det) < tol)
	{
		return false;
	}
	else
	{
		float inv_det = 1.0f / det;
		float c0 = (n11 * this->d() - n01 * p2.d())* inv_det;
		float c1 = (n00 * p2.d() - n01 * this->d())* inv_det;
		outLine.m = vector::cross3(n0, n1);
		outLine.b = n0 * c0 + n1 * c1;
		return true;
	}
}

//------------------------------------------------------------------------------
/**
*/
inline plane
plane::normalize(const plane& p)
{
	float4 f(p.vec);
	scalar len = f.length3();
	if(len < N_TINY )
	{
		return p;
	}
	f *= 1.0f/len;
	plane ret;
	ret.vec = f;
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline void
plane::set_a(scalar a)
{
    this->vec.set_x(a);
}

//------------------------------------------------------------------------------
/**
*/
inline void
plane::set_b(scalar b)
{
    this->vec.set_y(b);
}

//------------------------------------------------------------------------------
/**
*/
inline void
plane::set_c(scalar c)
{
    this->vec.set_z(c);
}

//------------------------------------------------------------------------------
/**
*/
inline void
plane::set_d(scalar d)
{
    this->vec.set_w(d);
}

//------------------------------------------------------------------------------
/**
*/
inline ClipStatus::Type
plane::clip(const line& l, line& clippedLine) const
{
    n_assert(&l != &clippedLine);
    float d0 = this->dot(l.start());
    float d1 = this->dot(l.end());
    if ((d0 >= N_TINY) && (d1 >= N_TINY))
    {
        // start and end point above plane
        clippedLine = l;
        return ClipStatus::Inside;
    }
    else if ((d0 < N_TINY) && (d1 < N_TINY))
    {
        // start and end point below plane
        return ClipStatus::Outside;
    }
    else
    {
        // line is clipped
        point clipPoint;
        this->intersectline(l.start(), l.end(), clipPoint);
        if (d0 >= N_TINY)
        {
            clippedLine.set(l.start(), clipPoint);
        }
        else
        {
            clippedLine.set(clipPoint, l.end());
        }
        return ClipStatus::Clipped;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline Math::float4
plane::get_normal() const
{
	float4 n = this->vec;
	n.set_w(0);
	return n;
}

//------------------------------------------------------------------------------
/**
*/
inline Math::float4
plane::get_point() const
{
	return this->get_normal() * -this->d();
}


} // namespace Math
//------------------------------------------------------------------------------
