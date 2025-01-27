#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::MeshBuilder
    
    A mesh builder utility class. Useful for exporter and converter tools.
    
    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "model/meshutil/meshbuildervertex.h"
#include "model/meshutil/meshbuildertriangle.h"
#include "model/meshutil/meshbuildergroup.h"
#include "util/fixedarray.h"
#include "util/array.h"
#include "math/bbox.h"
#include "coregraphics/primitivetopology.h"
#include "coregraphics/vertexlayout.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
class MeshBuilder
{
    struct Mesh;
public:

    /// constructor
    MeshBuilder();
    
    /// reserve dynamic arrays
    void NewMesh(SizeT numVertices, SizeT numTriangles);

    /// Set component mask
    void SetComponents(const MeshBuilderVertex::ComponentMask mask);
    /// Get components
    const MeshBuilderVertex::ComponentMask GetComponents();

    /// add a vertex
    void AddVertex(const MeshBuilderVertex& v);
    /// get number of vertices
    SizeT GetNumVertices() const;
    /// get vertex at index
    MeshBuilderVertex& VertexAt(IndexT i) const;

    /// add a triangle
    void AddTriangle(const MeshBuilderTriangle& t);
    /// get number of triangles
    SizeT GetNumTriangles() const;
    /// get triangle at index
    MeshBuilderTriangle& TriangleAt(IndexT i) const;

    /// set primitive topology
    void SetPrimitiveTopology(const CoreGraphics::PrimitiveTopology::Code& p);
    /// get primitive topology
    const CoreGraphics::PrimitiveTopology::Code& GetPrimitiveTopology();

    /// Set groups
    void SetPrimitiveGroups(const Util::Array<MeshBuilderGroup>& groups);
    /// Get groups
    const Util::Array<MeshBuilderGroup>& GetPrimitiveGroups();
    /// Clear primitive groups
    void ClearPrimitiveGroups();

    /// copy triangle with its vertices, do not generate redundant vertices
    void CopyTriangle(const MeshBuilder& srcMesh, IndexT triIndex, Util::FixedArray<IndexT>& indexMap);
    /// compute overall bounding box
    Math::bbox ComputeBoundingBox() const;

    /// sort triangles by group id
    void SortTriangles();
    /// clear object (deletes internal arrays)
    void Clear();
    /// transform vertices
    void Transform(const Math::mat4& m);
    /// remove redundant vertices
    void Deflate(Util::FixedArray<Util::Array<IndexT> >* collapseMap);
    /// inflate mesh to 3 unique vertices per triangles, created redundant vertices
    void Inflate();
    /// flip v texture coordinates
    void FlipUvs();
    /// Cleanup
    void Cleanup(Util::Array<Util::Array<int> > *collapseMap);

    /// Merge mesh with this mesh. NOTE: This does not take care of primitive groups!
    void Merge(MeshBuilder const& srcMesh);

    /// helper function for calculating normals
    void CalculateNormals();
    /// helper function for calculating tangents and binormals
    void CalculateTangents();

private:
    /// a qsort() hook for generating a sorted index array
    static int __cdecl VertexSorter(const void* elm0, const void* elm1);
    static MeshBuilder* qsortData;
    friend class MeshBuilderSaver;
    friend class SkinPartitioner;
    friend class SkinFragment;
    friend class NFbxScene;

    Util::Array<MeshBuilderTriangle> triangles;
    CoreGraphics::PrimitiveTopology::Code topology;
    MeshBuilderVertex::ComponentMask componentMask;
    Util::Array<MeshBuilderVertex> vertices;
    Util::Array<MeshBuilderGroup> groups;
};

//------------------------------------------------------------------------------
/**
*/
inline void
MeshBuilder::AddVertex(const MeshBuilderVertex& v)
{
    this->vertices.Append(v);
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
MeshBuilder::GetNumVertices() const
{
    return this->vertices.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline MeshBuilderVertex&
MeshBuilder::VertexAt(IndexT i) const
{
    return this->vertices[i];
}

//------------------------------------------------------------------------------
/**
*/
inline void
MeshBuilder::AddTriangle(const MeshBuilderTriangle& t)
{
    this->triangles.Append(t);
}

//------------------------------------------------------------------------------
/**
*/
inline SizeT
MeshBuilder::GetNumTriangles() const
{
    return this->triangles.Size();
}

//------------------------------------------------------------------------------
/**
*/
inline MeshBuilderTriangle&
MeshBuilder::TriangleAt(IndexT i) const
{
    return this->triangles[i];
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------
    