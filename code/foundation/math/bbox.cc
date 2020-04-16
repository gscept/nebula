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
vec4
bbox::corner_point(int index) const
{
    n_assert((index >= 0) && (index < 8));
    switch (index)
    {
        case 0:     return vec4(this->pmin, 1);
        case 1:     return vec4(this->pmin.x, this->pmax.y, this->pmin.z, 1);
        case 2:     return vec4(this->pmax.x, this->pmax.y, this->pmin.z, 1);
        case 3:     return vec4(this->pmax.x, this->pmin.y, this->pmin.z, 1);
        case 4:     return vec4(this->pmax, 1);
        case 5:     return vec4(this->pmin.x, this->pmax.y, this->pmax.z, 1);
        case 6:     return vec4(this->pmin.x, this->pmin.y, this->pmax.z, 1);
        default:    return vec4(this->pmax.x, this->pmin.y, this->pmax.z, 1);
    }    
}

//------------------------------------------------------------------------------
/**
    Get the bounding box's side planes in clip space.
*/
void
bbox::get_clipplanes(const mat4& viewProj, Util::Array<vec4>& outPlanes) const
{
    mat4 inverseTranspose = transpose(inverse(viewProj));
    vec4 planes[6];
	planes[0].set(-1, 0, 0, +this->pmax.x);
	planes[1].set(+1, 0, 0, -this->pmin.x);
	planes[2].set(0, -1, 0, +this->pmax.y);
	planes[3].set(0, +1, 0, -this->pmin.y);
	planes[4].set(0, 0, -1, +this->pmax.z);
	planes[5].set(0, 0, +1, -this->pmin.z);
    IndexT i;
    for (i = 0; i < 6; i++)
    {
        outPlanes.Append(inverseTranspose * planes[i]);
    }
}


} // namespace Math