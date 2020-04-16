#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::frustum
    
    Defines a clipping frustum made of 6 planes.
    
    (C) 2010 Radon Labs GmbH
    (C) 2013-2020 Individual contributors, see AUTHORS file
*/
#include "math/plane.h"
#include "math/matrix44.h"
#include "math/bbox.h"
#include "math/point.h"
#include "math/clipstatus.h"

//------------------------------------------------------------------------------
namespace Math
{
class frustum
{
public:
    /// plane indices
    enum PlaneIndex
    {
        Near = 0,
        Far,
        Left,
        Right,
        Top,
        Bottom,

        NumPlanes
    };

    /// default constructor
    frustum();
    /// construct from view and projection matrix
    frustum(const mat4& invViewProj);
    /// setup from view and proj matrix
    void set(const mat4& invViewProj);
    /// setup from transformed bounding box
    void set(const bbox& box, const mat4& boxTransform);
    /// test if point is inside frustum
    bool inside(const vec3& p) const;
    /// get clip bitmask of point (0 if inside, (1<<PlaneIndex) if outside)
    uint clipmask(const vec3& p) const;
    /// clip line against view frustum
    ClipStatus::Type clip(const line& l, line& clippedLine) const;
    /// get clip status of a local bounding box
    ClipStatus::Type clipstatus(const bbox& box) const;
    /// get clip status of a transformed bounding box
    ClipStatus::Type clipstatus(const bbox& box, const mat4& boxTransform) const;
    /// convert to any type
    template<typename T> T as() const;

    static const int TopLeftFar = 0;
    static const int TopRightFar = 1;
    static const int BottomLeftFar = 2;
    static const int BottomRightFar = 3;
    static const int TopLeftNear = 4;
    static const int TopRightNear = 5;
    static const int BottomLeftNear = 6;
    static const int BottomRightNear = 7;

    vec4 planes[NumPlanes];    
};        

//------------------------------------------------------------------------------
/**
*/
inline
frustum::frustum()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
inline 
frustum::frustum(const mat4& invViewProj)
{
    this->set(invViewProj);
}

//------------------------------------------------------------------------------
/**
    Setup frustum from invViewProj matrix (transform from projection space
    into world space).
*/
inline void
frustum::set(const mat4& invViewProj)
{
    // frustum corners in projection space
    vec4 projPoints[8];
    projPoints[TopLeftFar].set(-1.0f, 1.0f, 1.0f, 1);
    projPoints[TopRightFar].set(1.0f, 1.0f, 1.0f, 1);
    projPoints[BottomLeftFar].set(-1.0f, -1.0f, 1.0f, 1);
    projPoints[BottomRightFar].set(1.0f, -1.0f, 1.0f, 1);
    projPoints[TopLeftNear].set(-1.0f, 1.0f, 0.0f, 1);
    projPoints[TopRightNear].set(1.0f, 1.0f, 0.0f, 1);
    projPoints[BottomLeftNear].set(-1.0f, -1.0f, 0.0f, 1);
    projPoints[BottomRightNear].set(1.0f, -1.0f, 0.0f, 1);

    // compute frustum corners in world space
    vec3 worldPoints[8];
    IndexT i;
    for (i = 0; i < 8; i++)
    {
        vec4 p = invViewProj * projPoints[i];
        p *= 1.0f / p.w;
        worldPoints[i] = xyz(p);
    }

    // setup planes
    this->planes[Near] = plane::setup_from_points(worldPoints[TopRightNear], worldPoints[TopLeftNear], worldPoints[BottomLeftNear]);
    this->planes[Far] = plane::setup_from_points(worldPoints[TopLeftFar], worldPoints[TopRightFar], worldPoints[BottomRightFar]);
    this->planes[Left] = plane::setup_from_points(worldPoints[BottomLeftFar], worldPoints[BottomLeftNear], worldPoints[TopLeftNear]);
    this->planes[Right] = plane::setup_from_points(worldPoints[TopRightFar], worldPoints[TopRightNear], worldPoints[BottomRightNear]);
    this->planes[Top] = plane::setup_from_points(worldPoints[TopLeftNear], worldPoints[TopRightNear], worldPoints[TopRightFar]);
    this->planes[Bottom] = plane::setup_from_points(worldPoints[BottomLeftFar], worldPoints[BottomRightFar], worldPoints[BottomRightNear]);
}

//------------------------------------------------------------------------------
/**
    Setup from a transformed bounding box.
*/
inline void
frustum::set(const bbox& box, const mat4& boxTransform)
{
    // compute frustum corners in world space
    vec4 localPoint;
    vec3 worldPoints[8];
    IndexT i;
    for (i = 0; i < 8; i++)
    {
        // Top: pmax.y, Bottom: pmin.y, Left: pmin.x, Right: pmax.x, Far: pmax.z, Near: pmin.z
        switch (i)
        {
            // FIXME: replace with permute!
            case TopLeftFar:        localPoint.set(box.pmin.x, box.pmax.y, box.pmax.z, 1); break;
            case TopRightFar:       localPoint.set(box.pmax.x, box.pmax.y, box.pmax.z, 1); break;
            case BottomLeftFar:     localPoint.set(box.pmin.x, box.pmin.y, box.pmax.z, 1); break;
            case BottomRightFar:    localPoint.set(box.pmax.x, box.pmin.y, box.pmax.z, 1); break;
            case TopLeftNear:       localPoint.set(box.pmin.x, box.pmax.y, box.pmin.z, 1); break;
            case TopRightNear:      localPoint.set(box.pmax.x, box.pmax.y, box.pmin.z, 1); break;
            case BottomLeftNear:    localPoint.set(box.pmin.x, box.pmin.y, box.pmin.z, 1); break;
            case BottomRightNear:   localPoint.set(box.pmax.x, box.pmin.y, box.pmin.z, 1); break;
        }
        worldPoints[i] = xyz(boxTransform * localPoint);
    }

    // setup planes from transformed world space coordinates 
    this->planes[Near] = plane::setup_from_points(worldPoints[TopLeftNear], worldPoints[TopRightNear], worldPoints[BottomLeftNear]);
    this->planes[Far] = plane::setup_from_points(worldPoints[TopRightFar], worldPoints[TopLeftFar], worldPoints[BottomRightFar]);
    this->planes[Left] = plane::setup_from_points(worldPoints[BottomLeftNear], worldPoints[BottomLeftFar], worldPoints[TopLeftNear]);
    this->planes[Right] = plane::setup_from_points(worldPoints[BottomRightNear], worldPoints[TopRightNear], worldPoints[TopRightFar]);
    this->planes[Top] = plane::setup_from_points(worldPoints[TopRightNear], worldPoints[TopLeftNear], worldPoints[TopRightFar]);
    this->planes[Bottom] = plane::setup_from_points(worldPoints[BottomRightFar], worldPoints[BottomLeftFar], worldPoints[BottomRightNear]);
}

//------------------------------------------------------------------------------
/**
    Test if point is inside frustum.
*/
inline bool
frustum::inside(const vec3& p) const
{
    IndexT i;
    for (i = 0; i < NumPlanes; i++)
    {
        if (dot(xyz(this->planes[i]), p) > 0.0f)
        {
            return false;
        }
    }
    return true;
}

//------------------------------------------------------------------------------
/**
    Get clipmask of point.
*/
inline uint
frustum::clipmask(const vec3& p) const
{
    uint clipMask = 0;
    IndexT i;
    for (i = 0; i < NumPlanes; i++)
    {
        if (dot(xyz(this->planes[i]), p) > 0.0f)
        {
            clipMask |= 1<<i;
        }
    }
    return clipMask;
}

//------------------------------------------------------------------------------
/**
*/
inline ClipStatus::Type
frustum::clip(const line& l, line& clippedLine) const
{
    ClipStatus::Type clipStatus = ClipStatus::Inside;
    line l0(l);
    line l1;
    IndexT i;
    for (i = 0; i < NumPlanes; i++)
    {
        ClipStatus::Type planeClipStatus = plane::clip(this->planes[i], l0, l1);
        if (ClipStatus::Outside == planeClipStatus)
        {
            return ClipStatus::Outside;
        }
        else if (ClipStatus::Clipped == planeClipStatus)
        {
            clipStatus = ClipStatus::Clipped;
        }
        l0 = l1;
    }
    clippedLine = l0;
    return clipStatus;
}

//------------------------------------------------------------------------------
/**
*/
inline ClipStatus::Type
frustum::clipstatus(const bbox& box) const
{
    uint andFlags = 0xffff;
    uint orFlags = 0;
    vec3 p;
    IndexT i;
    for (i = 0; i < 8; i++)
    {
        // get corner point of bounding box
        switch (i)
        {
            // FIXME: REPLACE WITH PERMUTE!
            case 0:     p = box.pmin; break;
            case 1:     p.set(box.pmin.x, box.pmax.y, box.pmin.z); break;
            case 2:     p.set(box.pmax.x, box.pmax.y, box.pmin.z); break;
            case 3:     p.set(box.pmax.x, box.pmin.y, box.pmin.z); break;
            case 4:     p = box.pmax; break;
            case 5:     p.set(box.pmin.x, box.pmax.y, box.pmax.z); break;
            case 6:     p.set(box.pmin.x, box.pmin.y, box.pmax.z); break;
            case 7:     p.set(box.pmax.x, box.pmin.y, box.pmax.z); break;
        }

        // get clip mask of current box corner against frustum
        uint clipMask = this->clipmask(p);
        andFlags &= clipMask;
        orFlags  |= clipMask;
    }
    if (0 == orFlags)       return ClipStatus::Inside;
    else if (0 != andFlags) return ClipStatus::Outside;
    else                    return ClipStatus::Clipped;
}

//------------------------------------------------------------------------------
/**
    Returns the clip status of a transformed bounding box.
*/
inline ClipStatus::Type
frustum::clipstatus(const bbox& box, const mat4& boxTransform) const
{
    uint andFlags = 0xffff;
    uint orFlags = 0;
    vec4 localPoint, transformedPoint;
    IndexT i;
    for (i = 0; i < 8; i++)
    {
        // get corner point of bounding box
        switch (i)
        {
            // FIXME: REPLACE WITH PERMUTE!
            case 0:     localPoint = vec4(box.pmin, 1); break;
            case 1:     localPoint.set(box.pmin.x, box.pmax.y, box.pmin.z, 1); break;
            case 2:     localPoint.set(box.pmax.x, box.pmax.y, box.pmin.z, 1); break;
            case 3:     localPoint.set(box.pmax.x, box.pmin.y, box.pmin.z, 1); break;
            case 4:     localPoint = vec4(box.pmax, 1); break;
            case 5:     localPoint.set(box.pmin.x, box.pmax.y, box.pmax.z, 1); break;
            case 6:     localPoint.set(box.pmin.x, box.pmin.y, box.pmax.z, 1); break;
            case 7:     localPoint.set(box.pmax.x, box.pmin.y, box.pmax.z, 1); break;
        }

        // transform bounding box point
        transformedPoint = boxTransform * localPoint;

        // get clip mask of current box corner against frustum
        uint clipMask = this->clipmask(xyz(transformedPoint));
        andFlags &= clipMask;
        orFlags  |= clipMask;
    }
    if (0 == orFlags)       return ClipStatus::Inside;
    else if (0 != andFlags) return ClipStatus::Outside;
    else                    return ClipStatus::Clipped;
}

} // namespace Math
//------------------------------------------------------------------------------
    