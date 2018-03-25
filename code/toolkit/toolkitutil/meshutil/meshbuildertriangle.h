#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::MeshBuilderTriangle
    
    Contains triangle data in a mesh builder object.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "core/types.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class MeshBuilderTriangle
{
public:
    /// default constructor
    MeshBuilderTriangle();
    /// constructor
    MeshBuilderTriangle(IndexT i0, IndexT i1, IndexT i2, IndexT groupId=0);

    /// equality operator
    bool operator==(const MeshBuilderTriangle& rhs) const;
    /// inequality operator
    bool operator!=(const MeshBuilderTriangle& rhs) const;
    /// less-then operator
    bool operator<(const MeshBuilderTriangle& rhs) const;
    /// greater-then operator
    bool operator>(const MeshBuilderTriangle& rhs) const;
        
    /// compare 2 triangles, return -1, 0 or +1
    int Compare(const MeshBuilderTriangle& rhs) const;
    /// set vertex indices
    void SetVertexIndices(IndexT i0, IndexT i1, IndexT i2);
    /// get vertex indices
    void GetVertexIndices(IndexT& i0, IndexT& i1, IndexT& i2) const;
    /// set a single vertex index
    void SetVertexIndex(IndexT cornerIndex, IndexT vertexIndex);
    /// get a single vertex index
    IndexT GetVertexIndex(IndexT cornerIndex) const;
    /// set triangle group id
    void SetGroupId(IndexT id);
    /// get triangle group id
    IndexT GetGroupId() const;

private:
    friend class MeshBuilder;

    IndexT vertexIndex[3];
    IndexT groupId;
};

} // namespace ToolkitUtil
//------------------------------------------------------------------------------

    