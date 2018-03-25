//------------------------------------------------------------------------------
//  meshbuildertriangle.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitutil/meshutil/meshbuildertriangle.h"

namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
MeshBuilderTriangle::MeshBuilderTriangle() :
    groupId(0)
{
    this->vertexIndex[0] = 0;
    this->vertexIndex[1] = 0;
    this->vertexIndex[2] = 0;
}

//------------------------------------------------------------------------------
/**
*/
MeshBuilderTriangle::MeshBuilderTriangle(IndexT i0, IndexT i1, IndexT i2, IndexT groupId_)
{
    this->vertexIndex[0] = i0;
    this->vertexIndex[1] = i1;
    this->vertexIndex[2] = i2;
    this->groupId = groupId_;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderTriangle::SetVertexIndices(IndexT i0, IndexT i1, IndexT i2)
{
    this->vertexIndex[0] = i0;
    this->vertexIndex[1] = i1;
    this->vertexIndex[2] = i2;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderTriangle::GetVertexIndices(IndexT& i0, IndexT& i1, IndexT& i2) const
{
    i0 = this->vertexIndex[0];
    i1 = this->vertexIndex[1];
    i2 = this->vertexIndex[2];
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderTriangle::SetVertexIndex(IndexT cornerIndex, IndexT vertexIndex)
{
    n_assert((cornerIndex >= 0) && (cornerIndex < 3));
    this->vertexIndex[cornerIndex] = vertexIndex;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
MeshBuilderTriangle::GetVertexIndex(IndexT cornerIndex) const
{
    n_assert((cornerIndex >= 0) && (cornerIndex < 3));
    return this->vertexIndex[cornerIndex];
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderTriangle::SetGroupId(IndexT id)
{
    this->groupId = id;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
MeshBuilderTriangle::GetGroupId() const
{
    return this->groupId;
}

//------------------------------------------------------------------------------
/**
*/
int
MeshBuilderTriangle::Compare(const MeshBuilderTriangle& rhs) const
{
    // compare group id
    if (this->groupId > rhs.groupId)
    {
        return 1;
    }
    else if (this->groupId < rhs.groupId)
    {
        return -1;
    }

    // compare vertex indices if group id is identical
    IndexT i;
    for (i = 0; i < 3; i++)
    {
        if (this->vertexIndex[i] > rhs.vertexIndex[i])
        {
            return 1;
        }
        else if (this->vertexIndex[i] < rhs.vertexIndex[i])
        {
            return -1;
        }
    }

    // FIXME: For some reason it can happen that 2 triangles
    // with identical indices are exported. Make sure we
    // still get a definitive sorting order.
    // (however, duplicate triangles should not be generated
    // in the first place).
    if (this > &rhs)
    {
        return 1;
    }
    else if (this < &rhs)
    {
        return -1;
    }
    else
    {
        return 0;
    }
}

//------------------------------------------------------------------------------
/**
*/
bool
MeshBuilderTriangle::operator==(const MeshBuilderTriangle& rhs) const
{
    return (0 == this->Compare(rhs));
}

//------------------------------------------------------------------------------
/**
*/
bool
MeshBuilderTriangle::operator!=(const MeshBuilderTriangle& rhs) const
{
    return (0 != this->Compare(rhs));
}

//------------------------------------------------------------------------------
/**
*/
bool
MeshBuilderTriangle::operator<(const MeshBuilderTriangle& rhs) const
{
    return (-1 == this->Compare(rhs));
}

//------------------------------------------------------------------------------
/**
*/
bool
MeshBuilderTriangle::operator>(const MeshBuilderTriangle& rhs) const
{
    return (+1 == this->Compare(rhs));
}

} // namespace ToolkitUtil