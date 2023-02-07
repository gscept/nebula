#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::MeshBuilderGroup
    
    Defines a group of vertices in a mesh builder object.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class MeshBuilderGroup
{
public:
    /// constructor
    MeshBuilderGroup();
    /// set first triangle index
    void SetFirstTriangleIndex(IndexT i);
    /// get first triangle index
    IndexT GetFirstTriangleIndex() const;
    /// set number of triangles in group
    void SetNumTriangles(SizeT n);
    /// get number of triangles in group
    SizeT GetNumTriangles() const;

private:
    IndexT firstTriangleIndex;
    SizeT numTriangles;
};

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    