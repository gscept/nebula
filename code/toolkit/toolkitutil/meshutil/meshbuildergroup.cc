//------------------------------------------------------------------------------
//  meshbuildergroup.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "toolkitutil/meshutil/meshbuildergroup.h"

namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
MeshBuilderGroup::MeshBuilderGroup() :
    groupId(0),
    firstTriangleIndex(0),
    numTriangles(0)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderGroup::SetGroupId(IndexT id)
{
    this->groupId = id;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
MeshBuilderGroup::GetGroupId() const
{
    return this->groupId;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderGroup::SetFirstTriangleIndex(IndexT i)
{
    this->firstTriangleIndex = i;
}

//------------------------------------------------------------------------------
/**
*/
IndexT
MeshBuilderGroup::GetFirstTriangleIndex() const
{
    return this->firstTriangleIndex;
}

//------------------------------------------------------------------------------
/**
*/
void
MeshBuilderGroup::SetNumTriangles(SizeT n)
{
    this->numTriangles = n;
}

//------------------------------------------------------------------------------
/**
*/
SizeT
MeshBuilderGroup::GetNumTriangles() const
{
    return this->numTriangles;
}

} // namespace ToolkitUtil
