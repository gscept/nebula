#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxMeshNode

    Wraps an FBX mesh node into a Nebula-style fbx node

    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "ngltfnode.h"
#include "math/bbox.h"
#include "meshutil/meshbuilder.h"
#include "toolkit-common/base/exporttypes.h"
#include "fbx/character/skinfragment.h"
#include "meshprimitive.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{
//class NglTFJointNode;
class NglTFMesh : public NglTFNode
{
    __DeclareClass(NglTFMesh);
public:

    typedef uint MeshMask;

    /// constructor
    NglTFMesh();
    /// destructor
    virtual ~NglTFMesh();

    /// sets up node from assimp-node containing a mesh
    void Setup(Gltf::Node const* node, const Ptr<NglTFScene>& scene);
    /// discard node and its mesh
    void Discard();

    /// sets the mesh bounding box
    void SetBoundingBox(const Math::bbox& bbox);
    /// returns mesh node bounding box
    const Math::bbox& GetBoundingBox() const;

    /// returns pointer to mesh source
    ToolkitUtil::MeshBuilder* GetMesh() const;
    /// replace internal mesh pointer with given
    void SetMesh(ToolkitUtil::MeshBuilder* newMesh);
    
    /// retrieve MeshPrimitive array
    Util::Array<MeshPrimitive>& Primitives();

protected:
    friend class NglTFScene;

    /// extracts mesh data
    void ExtractMesh();

    /// helper function for calculating normals
    void CalculateNormals();
    /// helper function for calculating tangents and binormals
    void CalculateTangentsAndBinormals();

    int32_t meshId;
    Gltf::Mesh const* gltfMesh;
    ToolkitUtil::MeshBuilder* meshBuilder;
    Util::Array<MeshPrimitive> primGroups;
    Util::Array<Ptr<ToolkitUtil::SkinFragment>> skinFragments;
    Math::bbox boundingBox;
};

//------------------------------------------------------------------------------
/**
*/
inline void
NglTFMesh::SetBoundingBox(const Math::bbox& bbox)
{
    this->boundingBox = bbox;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::bbox&
NglTFMesh::GetBoundingBox() const
{
    return this->boundingBox;
}

//------------------------------------------------------------------------------
/**
*/
inline ToolkitUtil::MeshBuilder*
NglTFMesh::GetMesh() const
{
    return this->meshBuilder;
}


//------------------------------------------------------------------------------
/**
*/
inline void
NglTFMesh::SetMesh(ToolkitUtil::MeshBuilder* newMesh)
{
    this->meshBuilder = newMesh;
}

//------------------------------------------------------------------------------
/**
*/
inline Util::Array<MeshPrimitive>&
NglTFMesh::Primitives()
{
    return this->primGroups;
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------