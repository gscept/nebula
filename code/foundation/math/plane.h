#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::plane

    Nebula's plane class.

    (C) 2020 Individual contributors, see AUTHORS file

*/
//------------------------------------------------------------------------------
#include "math/point.h"
#include "math/vector.h"
#include "math/line.h"
#include "scalar.h"
#include "clipstatus.h"

namespace Math
{
struct plane
{

    /// default constructor, NOTE: does NOT setup components!
    plane() = default;
    /// construct from values
    plane(scalar a, scalar b, scalar c, scalar d);
    /// setup from point and normal
    plane(const point& p, const vector& n);
    /// setup from points
    plane(const point& p0, const point& p1, const point& p2);
    /// construct from SSE 128 byte float array
    plane(const __m128 & rhs);

    /// set content
    void set(scalar a, scalar b, scalar c, scalar d);

    union
    {
        __m128 vec;
        struct
        {
            float a, b, c, d;
        };
    };
};

//------------------------------------------------------------------------------
/**
*/
__forceinline
plane::plane(scalar a, scalar b, scalar c, scalar d)
{
    this->vec = _mm_setr_ps(a, b, c, d);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
plane::plane(const point& p, const vector& n)
{
    float d = dot(p, n);
    this->vec = _mm_setr_ps(n.x, n.y, n.z, -d);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
plane::plane(const point& p0, const point& p1, const point& p2)
{
    vector v21 = p0 - p1;
    vector v31 = p0 - p2;

    vector cr = cross(v21, v31);
    cr = normalize(cr);
    float d = dot(cr, p0);

    this->vec = _mm_setr_ps(cr.x, cr.y, cr.z, -d);
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
plane::plane(const __m128& rhs)
{
    this->vec = rhs;
}

//------------------------------------------------------------------------------
/**
*/
__forceinline
void plane::set(scalar a, scalar b, scalar c, scalar d)
{
    this->vec = _mm_setr_ps(a, b, c, d);
}

//------------------------------------------------------------------------------
/**
*/
inline vector 
get_normal(const plane& plane)
{
    vector res;
    res.vec = _mm_and_ps(plane.vec, _mask_xyz);
    return res;
}

//------------------------------------------------------------------------------
/**
*/
inline point
get_point(const plane& plane)
{
    return _mm_mul_ps(get_normal(plane).vec, _mm_set1_ps(-plane.d));
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
intersectline(const plane& plane, const point& startPoint, const point& endPoint, point& outIntersectPoint)
{
    scalar v1 = dot(get_normal(plane), startPoint);
    scalar v2 = dot(get_normal(plane), endPoint);
    scalar d = (v1 - v2);
    if (n_abs(d) < N_TINY)
    {
        return false;
    }

    d = 1.0f / d;

    scalar pd = dot(get_normal(plane), startPoint);
    pd *= d;

    vec4 p = (endPoint - startPoint) * pd;

    p += startPoint;
    outIntersectPoint = p;

    return true;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
intersectplane(const plane& p1, const plane& p2, line& outLine)
{
    vector n0 = get_normal(p1);
    vector n1 = get_normal(p2);
    float n00 = dot(n0, n0);
    float n01 = dot(n0, n1);
    float n11 = dot(n1, n1);
    float det = n00 * n11 - n01 * n01;
    const float tol = N_TINY;
    if (fabs(det) < tol)
    {
        return false;
    }
    else
    {
        float inv_det = 1.0f / det;
        float c0 = (n11 * p1.d - n01 * p2.d) * inv_det;
        float c1 = (n00 * p2.d - n01 * p1.d) * inv_det;
        outLine.m = cross(n0, n1);
        outLine.b = vec4(n0 * c0 + n1 * c1, 1);
        return true;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline ClipStatus::Type 
clip(const plane& plane, const line& l, line& outClippedLine)
{
    n_assert(&l != &outClippedLine);
    float d0 = dot(get_normal(plane), l.start());
    float d1 = dot(get_normal(plane), l.end());
    if ((d0 >= N_TINY) && (d1 >= N_TINY))
    {
        // start and end point above plane
        outClippedLine = l;
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
        intersectline(plane, l.start(), l.end(), clipPoint);
        if (d0 >= N_TINY)
        {
            outClippedLine.set(l.start(), clipPoint);
        }
        else
        {
            outClippedLine.set(clipPoint, l.end());
        }
        return ClipStatus::Clipped;
    }
}


//------------------------------------------------------------------------------
/**
*/
__forceinline scalar
dot(const plane& p, const vec4& v1)
{
    return _mm_cvtss_f32(_mm_dp_ps(p.vec, v1.vec, 0xF1));
}

//------------------------------------------------------------------------------
/**
*/
__forceinline plane
normalize(const plane& p)
{
    vec4 f(p.vec);
    scalar len = length3(f);
    if (len < N_TINY)
    {
        return p;
    }
    f *= 1.0f / len;
    plane ret;
    ret.vec = f.vec;
    return ret;
}


//------------------------------------------------------------------------------
/**
*/
__forceinline plane
operator*(const mat4& m, const plane& p)
{
    __m128 x = _mm_shuffle_ps(p.vec, p.vec, _MM_SHUFFLE(0, 0, 0, 0));
    __m128 y = _mm_shuffle_ps(p.vec, p.vec, _MM_SHUFFLE(1, 1, 1, 1));
    __m128 z = _mm_shuffle_ps(p.vec, p.vec, _MM_SHUFFLE(2, 2, 2, 2));
    __m128 w = _mm_shuffle_ps(p.vec, p.vec, _MM_SHUFFLE(3, 3, 3, 3));

    return _mm_add_ps(
        _mm_add_ps(_mm_mul_ps(x, m.r[0].vec), _mm_mul_ps(y, m.r[1].vec)),
        _mm_add_ps(_mm_mul_ps(z, m.r[2].vec), _mm_mul_ps(w, m.r[3].vec))
    );
}

} // namespace Math