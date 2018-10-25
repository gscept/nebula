#pragma once
//------------------------------------------------------------------------------
/**
    @class Math::frustum
    
    Defines a clipping frustum made of 6 planes.
    
    (C) 2010 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
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
    frustum(const matrix44& invViewProj);
    /// setup from view and proj matrix
    void set(const matrix44& invViewProj);
    /// setup from transformed bounding box
    void set(const bbox& box, const matrix44& boxTransform);
    /// test if point is inside frustum
    bool inside(const point& p) const;
    /// get clip bitmask of point (0 if inside, (1<<PlaneIndex) if outside)
    uint clipmask(const point& p) const;
    /// clip line against view frustum
    ClipStatus::Type clip(const line& l, line& clippedLine) const;
    /// get clip status of a local bounding box
    ClipStatus::Type clipstatus(const bbox& box) const;
    /// get clip status of a transformed bounding box
    ClipStatus::Type clipstatus(const bbox& box, const matrix44& boxTransform) const;
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

    plane planes[NumPlanes];    
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
frustum::frustum(const matrix44& invViewProj)
{
    this->set(invViewProj);
}

//------------------------------------------------------------------------------
/**
    Setup frustum from invViewProj matrix (transform from projection space
    into world space).
*/
inline void
frustum::set(const matrix44& invViewProj)
{
    // frustum corners in projection space
    point projPoints[8];
    projPoints[TopLeftFar].set(-1.0f, 1.0f, 1.0f);
    projPoints[TopRightFar].set(1.0f, 1.0f, 1.0f);
    projPoints[BottomLeftFar].set(-1.0f, -1.0f, 1.0f);
    projPoints[BottomRightFar].set(1.0f, -1.0f, 1.0f);
    projPoints[TopLeftNear].set(-1.0f, 1.0f, 0.0f);
    projPoints[TopRightNear].set(1.0f, 1.0f, 0.0f);
    projPoints[BottomLeftNear].set(-1.0f, -1.0f, 0.0f);
    projPoints[BottomRightNear].set(1.0f, -1.0f, 0.0);

    // compute frustum corners in world space
    point worldPoints[8];
    IndexT i;
    for (i = 0; i < 8; i++)
    {
        point p = matrix44::transform(projPoints[i], invViewProj);
        p *= 1.0f / p.w();
    }

    // setup planes
    this->planes[Near].setup_from_points(worldPoints[TopRightNear], worldPoints[TopLeftNear], worldPoints[BottomLeftNear]);
    this->planes[Far].setup_from_points(worldPoints[TopLeftFar], worldPoints[TopRightFar], worldPoints[BottomRightFar]);
    this->planes[Left].setup_from_points(worldPoints[BottomLeftFar], worldPoints[BottomLeftNear], worldPoints[TopLeftNear]);
    this->planes[Right].setup_from_points(worldPoints[TopRightFar], worldPoints[TopRightNear], worldPoints[BottomRightNear]);
    this->planes[Top].setup_from_points(worldPoints[TopLeftNear], worldPoints[TopRightNear], worldPoints[TopRightFar]);
    this->planes[Bottom].setup_from_points(worldPoints[BottomLeftFar], worldPoints[BottomRightFar], worldPoints[BottomRightNear]);
}

//------------------------------------------------------------------------------
/**
    Setup from a transformed bounding box.
*/
inline void
frustum::set(const bbox& box, const matrix44& boxTransform)
{
    // compute frustum corners in world space
    point localPoint;
    point worldPoints[8];
    IndexT i;
    for (i = 0; i < 8; i++)
    {
        // Top: pmax.y, Bottom: pmin.y, Left: pmin.x, Right: pmax.x, Far: pmax.z, Near: pmin.z
        switch (i)
        {
            // FIXME: replace with permute!
            case TopLeftFar:        localPoint.set(box.pmin.x(), box.pmax.y(), box.pmax.z()); break;
            case TopRightFar:       localPoint.set(box.pmax.x(), box.pmax.y(), box.pmax.z()); break;
            case BottomLeftFar:     localPoint.set(box.pmin.x(), box.pmin.y(), box.pmax.z()); break;
            case BottomRightFar:    localPoint.set(box.pmax.x(), box.pmin.y(), box.pmax.z()); break;
            case TopLeftNear:       localPoint.set(box.pmin.x(), box.pmax.y(), box.pmin.z()); break;
            case TopRightNear:      localPoint.set(box.pmax.x(), box.pmax.y(), box.pmin.z()); break;
            case BottomLeftNear:    localPoint.set(box.pmin.x(), box.pmin.y(), box.pmin.z()); break;
            case BottomRightNear:   localPoint.set(box.pmax.x(), box.pmin.y(), box.pmin.z()); break;
        }
        worldPoints[i] = matrix44::transform(localPoint, boxTransform);
    }

    // setup planes from transformed world space coordinates 
    this->planes[Near].setup_from_points(worldPoints[TopLeftNear], worldPoints[TopRightNear], worldPoints[BottomLeftNear]);
    this->planes[Far].setup_from_points(worldPoints[TopRightFar], worldPoints[TopLeftFar], worldPoints[BottomRightFar]);
    this->planes[Left].setup_from_points(worldPoints[BottomLeftNear], worldPoints[BottomLeftFar], worldPoints[TopLeftNear]);
    this->planes[Right].setup_from_points(worldPoints[BottomRightNear], worldPoints[TopRightNear], worldPoints[TopRightFar]);
    this->planes[Top].setup_from_points(worldPoints[TopRightNear], worldPoints[TopLeftNear], worldPoints[TopRightFar]);
    this->planes[Bottom].setup_from_points(worldPoints[BottomRightFar], worldPoints[BottomLeftFar], worldPoints[BottomRightNear]);
}

//------------------------------------------------------------------------------
/**
    Test if point is inside frustum.
*/
inline bool
frustum::inside(const point& p) const
{
    IndexT i;
    for (i = 0; i < NumPlanes; i++)
    {
        if (this->planes[i].dot(p) > 0.0f)
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
frustum::clipmask(const point& p) const
{
    uint clipMask = 0;
    IndexT i;
    for (i = 0; i < NumPlanes; i++)
    {
        if (this->planes[i].dot(p) > 0.0f)
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
        ClipStatus::Type planeClipStatus = this->planes[i].clip(l0, l1);
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
    point p;
    IndexT i;
    for (i = 0; i < 8; i++)
    {
        // get corner point of bounding box
        switch (i)
        {
            // FIXME: REPLACE WITH PERMUTE!
            case 0:     p = box.pmin; break;
            case 1:     p.set(box.pmin.x(), box.pmax.y(), box.pmin.z()); break;
            case 2:     p.set(box.pmax.x(), box.pmax.y(), box.pmin.z()); break;
            case 3:     p.set(box.pmax.x(), box.pmin.y(), box.pmin.z()); break;
            case 4:     p = box.pmax; break;
            case 5:     p.set(box.pmin.x(), box.pmax.y(), box.pmax.z()); break;
            case 6:     p.set(box.pmin.x(), box.pmin.y(), box.pmax.z()); break;
            case 7:     p.set(box.pmax.x(), box.pmin.y(), box.pmax.z()); break;
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
frustum::clipstatus(const bbox& box, const matrix44& boxTransform) const
{
    uint andFlags = 0xffff;
    uint orFlags = 0;
    point localPoint, transformedPoint;
    IndexT i;
    for (i = 0; i < 8; i++)
    {
        // get corner point of bounding box
        switch (i)
        {
            // FIXME: REPLACE WITH PERMUTE!
            case 0:     localPoint = box.pmin; break;
            case 1:     localPoint.set(box.pmin.x(), box.pmax.y(), box.pmin.z()); break;
            case 2:     localPoint.set(box.pmax.x(), box.pmax.y(), box.pmin.z()); break;
            case 3:     localPoint.set(box.pmax.x(), box.pmin.y(), box.pmin.z()); break;
            case 4:     localPoint = box.pmax; break;
            case 5:     localPoint.set(box.pmin.x(), box.pmax.y(), box.pmax.z()); break;
            case 6:     localPoint.set(box.pmin.x(), box.pmin.y(), box.pmax.z()); break;
            case 7:     localPoint.set(box.pmax.x(), box.pmin.y(), box.pmax.z()); break;
        }

        // transform bounding box point
        transformedPoint = matrix44::transform(localPoint, boxTransform);

        // get clip mask of current box corner against frustum
        uint clipMask = this->clipmask(transformedPoint);
        andFlags &= clipMask;
        orFlags  |= clipMask;
    }
    if (0 == orFlags)       return ClipStatus::Inside;
    else if (0 != andFlags) return ClipStatus::Outside;
    else                    return ClipStatus::Clipped;
}

} // namespace Math
//------------------------------------------------------------------------------
    