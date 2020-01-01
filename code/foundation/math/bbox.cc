//------------------------------------------------------------------------------
//  bbox.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "math/bbox.h"

namespace Math
{

//------------------------------------------------------------------------------
/**
    Return box/box clip status.
*/
ClipStatus::Type
bbox::clipstatus(const bbox& other) const
{
    if (this->contains(other))
    {
        return ClipStatus::Inside;
    }
    else if (this->intersects(other))
    {
        return ClipStatus::Clipped;
    }
    else 
    {
        return ClipStatus::Outside;
    }
}

//------------------------------------------------------------------------------
/**
    Returns one of the 8 corners of the bounding box.
*/
point
bbox::corner_point(int index) const
{
    n_assert((index >= 0) && (index < 8));
    switch (index)
    {
        case 0:     return this->pmin;
        case 1:     return point(this->pmin.x(), this->pmax.y(), this->pmin.z());
        case 2:     return point(this->pmax.x(), this->pmax.y(), this->pmin.z());
        case 3:     return point(this->pmax.x(), this->pmin.y(), this->pmin.z());
        case 4:     return this->pmax;
        case 5:     return point(this->pmin.x(), this->pmax.y(), this->pmax.z());
        case 6:     return point(this->pmin.x(), this->pmin.y(), this->pmax.z());
        default:    return point(this->pmax.x(), this->pmin.y(), this->pmax.z());
    }    
}

//------------------------------------------------------------------------------
/**
    Get the bounding box's side planes in clip space.
*/
void
bbox::get_clipplanes(const matrix44& viewProj, Util::Array<plane>& outPlanes) const
{
    matrix44 inverseTranspose = matrix44::transpose(matrix44::inverse(viewProj));
    plane planes[6];
    planes[0].set(-1, 0, 0, +this->pmax.x());
    planes[1].set(+1, 0, 0, -this->pmin.x());
    planes[2].set(0, -1, 0, +this->pmax.y());
    planes[3].set(0, +1, 0, -this->pmin.y());
    planes[4].set(0, 0, -1, +this->pmax.z());
    planes[5].set(0, 0, +1, -this->pmin.z());
    IndexT i;
    for (i = 0; i < 6; i++)
    {
        outPlanes.Append(matrix44::transform(planes[i], inverseTranspose));
    }
}


} // namespace Math