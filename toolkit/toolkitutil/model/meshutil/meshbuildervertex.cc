//------------------------------------------------------------------------------
//  meshbuildervertex.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "meshbuildervertex.h"

namespace ToolkitUtil
{
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
MeshBuilderVertex::MeshBuilderVertex() :
    componentMask(Position)
    , flagMask(0)
{
    this->attributes.skin.indices = { 0, 0, 0, 0 };
}

//------------------------------------------------------------------------------
/**
    Returns +1 if this vertex is greater then other vertex, -1 if 
    this vertex is less then other vertex, 0 if vertices
    are equal.
*/
int
MeshBuilderVertex::Compare(const MeshBuilderVertex& rhs) const
{
    n_assert(this->componentMask == rhs.componentMask);

    if (greater_any(this->base.position, rhs.base.position))
        return 1;
    else if (less_any(this->base.position, rhs.base.position))
        return -1;

    if (this->base.uv.x > rhs.base.uv.x
        || this->base.uv.y > rhs.base.uv.y)
        return 1;
    else if (this->base.uv.x < rhs.base.uv.x
        || this->base.uv.y < rhs.base.uv.y)
        return -1;

    if (AllBits(this->componentMask, Components::Normals))
    {
        if (greater_any(this->attributes.normal.normal, rhs.attributes.normal.normal))
            return 1;
        else if (less_any(this->attributes.normal.normal, rhs.attributes.normal.normal))
            return -1;

        if (greater_any(this->attributes.normal.tangent, rhs.attributes.normal.tangent))
            return 1;
        else if (less_any(this->attributes.normal.tangent, rhs.attributes.normal.tangent))
            return -1;
    }
    if (AllBits(this->componentMask, Components::Color))
    {
        if (greater_any(this->attributes.color.color, rhs.attributes.color.color))
            return 1;
        else if (less_any(this->attributes.color.color, rhs.attributes.color.color))
            return -1;
    }
    if (AllBits(this->componentMask, Components::SecondUv))
    {
        if (this->attributes.secondUv.uv2.x > rhs.attributes.secondUv.uv2.x
    || this->attributes.secondUv.uv2.y > rhs.attributes.secondUv.uv2.y)
            return 1;
        else if (this->attributes.secondUv.uv2.x < rhs.attributes.secondUv.uv2.x
            || this->attributes.secondUv.uv2.y < rhs.attributes.secondUv.uv2.y)
            return -1;
    }
    if (AllBits(this->componentMask, Components::SkinIndices | Components::SkinWeights))
    {
        if (greater_any(this->attributes.skin.weights, rhs.attributes.skin.weights))
            return 1;
        else if (less_any(this->attributes.skin.weights, rhs.attributes.skin.weights))
            return -1;

        if (this->attributes.skin.indices.x > rhs.attributes.skin.indices.x
            || this->attributes.skin.indices.y > rhs.attributes.skin.indices.y
            || this->attributes.skin.indices.z > rhs.attributes.skin.indices.z
            || this->attributes.skin.indices.w > rhs.attributes.skin.indices.w)
            return 1;
        else if (this->attributes.skin.indices.x < rhs.attributes.skin.indices.x
            || this->attributes.skin.indices.y < rhs.attributes.skin.indices.y
            || this->attributes.skin.indices.z < rhs.attributes.skin.indices.z
            || this->attributes.skin.indices.w < rhs.attributes.skin.indices.w)
            return -1;
    }

    // fallthrough: all components equal
    return 0;
}

//------------------------------------------------------------------------------
/**
*/
bool
MeshBuilderVertex::operator==(const MeshBuilderVertex& rhs) const
{
    return (0 == this->Compare(rhs));
}

//------------------------------------------------------------------------------
/**
*/
bool
MeshBuilderVertex::operator!=(const MeshBuilderVertex& rhs) const
{
    return (0 != this->Compare(rhs));
}

//------------------------------------------------------------------------------
/**
*/
bool
MeshBuilderVertex::operator<(const MeshBuilderVertex& rhs) const
{
    return (-1 == this->Compare(rhs));
}

//------------------------------------------------------------------------------
/**
*/
bool
MeshBuilderVertex::operator>(const MeshBuilderVertex& rhs) const
{
    return (+1 == this->Compare(rhs));
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderVertex::Transform(const mat4& m)
{
    this->base.position = m * this->base.position;

    if (AllBits(this->componentMask, Components::Normals))
    {
        this->attributes.normal.normal = m * Math::vector(this->attributes.normal.normal);
        this->attributes.normal.normal = normalize(this->attributes.normal.normal);
        this->attributes.normal.tangent = m * Math::vector(this->attributes.normal.tangent);
        this->attributes.normal.tangent = normalize(this->attributes.normal.tangent);
    }
}

} // namespace ToolkitUtil