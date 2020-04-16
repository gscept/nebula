#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::plane

    Nebula's plane class.

    (C) 2020 Individual contributors, see AUTHORS file

*/
//------------------------------------------------------------------------------
#include "math2/vec4.h"
#include "math/line.h"
#include "clipstatus.h"

namespace Math
{
struct plane
{
    /// setup from points
    static vec4 setup_from_points(const vec3& p0, const vec3& p1, const vec3& p2);
    /// setup from point and normal
    static vec4 setup_from_point_and_normal(const vec3& p, const vec3& n);

    /// get the plane normal
    static vec3 get_normal(const vec4& plane);
    /// get the plane point
    static vec3 get_point(const vec4& plane);

    /// find intersection with line
    static bool intersectline(const vec4& plane, const vec3& startPoint, const vec3& endPoint, vec3& outIntersectPoint);
    /// find intersection with plane
    static bool intersectplane(const vec4& plane, const vec4& p2, line& outLine);
    /// clip line against this plane
    static ClipStatus::Type clip(const vec4& plane, const line& l, line& outClippedLine);
};

//------------------------------------------------------------------------------
/**
*/
inline vec4 
plane::setup_from_points(const vec3& p0, const vec3& p1, const vec3& p2)
{
    vec3 v21 = p0 - p1;
    vec3 v31 = p0 - p2;

    vec3 cr = cross(v21, v31);
    cr = normalize(cr);
    float d = dot(cr, p0);

    vec4 ret(cr, -d);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline vec4 
plane::setup_from_point_and_normal(const vec3& p, const vec3& n)
{
    float d = dot(p, n);
    vec4 ret(n, -d);
    return ret;
}

//------------------------------------------------------------------------------
/**
*/
inline vec3 
plane::get_normal(const vec4& plane)
{
    return xyz(plane);
}

//------------------------------------------------------------------------------
/**
*/
inline vec3
plane::get_point(const vec4& plane)
{
    return xyz(plane * -plane.w);
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
plane::intersectline(const vec4& plane, const vec3& startPoint, const vec3& endPoint, vec3& outIntersectPoint)
{
    scalar v1 = dot(xyz(plane), startPoint);
    scalar v2 = dot(xyz(plane), endPoint);
    scalar d = (v1 - v2);
    if (n_abs(d) < N_TINY)
    {
        return false;
    }

    d = 1.0f / d;

    scalar pd = dot(xyz(plane), startPoint);
    pd *= d;

    vec3 p = (endPoint - startPoint) * pd;

    p += startPoint;
    outIntersectPoint = p;

    return true;
}

//------------------------------------------------------------------------------
/**
*/
inline bool 
plane::intersectplane(const vec4& plane, const vec4& p2, line& outLine)
{
    vec3 n0 = get_normal(plane);
    vec3 n1 = get_normal(p2);
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
        float c0 = (n11 * plane.w - n01 * p2.w) * inv_det;
        float c1 = (n00 * p2.w - n01 * plane.w) * inv_det;
        outLine.m = cross(n0, n1);
        outLine.b = n0 * c0 + n1 * c1;
        return true;
    }
}

//------------------------------------------------------------------------------
/**
*/
inline ClipStatus::Type 
plane::clip(const vec4& plane, const line& l, line& outClippedLine)
{
    n_assert(&l != &outClippedLine);
    float d0 = dot(xyz(plane), l.start());
    float d1 = dot(xyz(plane), l.end());
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
        vec3 clipPoint;
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

} // namespace Math