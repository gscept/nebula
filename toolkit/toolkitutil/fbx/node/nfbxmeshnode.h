#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxMeshNode
    
    Wraps an FBX mesh node into a Nebula-style fbx node
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
#include "fbx/node/nfbxnode.h"
#include "math/bbox.h"
#include "meshutil/meshbuilder.h"
#include "toolkit-common/base/exporttypes.h"
#include "fbx/character/skinfragment.h"

//------------------------------------------------------------------------------
namespace ToolkitUtil
{

class NFbxJointNode;
class NFbxMeshNode : public NFbxNode
{
    __DeclareClass(NFbxMeshNode);
public:

    typedef uint MeshMask;

    /// constructor
    NFbxMeshNode();
    /// destructor
    virtual ~NFbxMeshNode();

    /// sets up node from kfbx-node containing a mesh
    void Setup(FbxNode* node, const Ptr<NFbxScene>& scene);
    /// discard node and its mesh
    void Discard();

    /// returns the mesh flags
    const ToolkitUtil::MeshFlags& GetMeshFlags() const;
    /// sets the parse flags
    void SetExportFlags(const ToolkitUtil::ExportFlags& flags);
    /// returns the export flags
    const ToolkitUtil::ExportFlags& GetExportFlags() const;
    /// sets the export mode
    void SetExportMode(const ToolkitUtil::ExportMode& mode);
    /// returns the export mode
    const ToolkitUtil::ExportMode& GetExportMode() const;

    /// sets the mesh bounding box
    void SetBoundingBox(const Math::bbox& bbox);
    /// returns mesh node bounding box
    const Math::bbox& GetBoundingBox() const;

    /// returns pointer to skeleton link
    Ptr<NFbxJointNode> GetSkeletonLink() const;
    /// returns pointer to mesh source
    ToolkitUtil::MeshBuilder* GetMesh() const;
    /// replace internal mesh pointer with given
    void SetMesh(ToolkitUtil::MeshBuilder* newMesh);
    /// returns reference to skin fragments
    const Util::Array<Ptr<ToolkitUtil::SkinFragment> >& GetSkinFragments() const;
    /// sets list of skin fragments
    void SetSkinFragments(const Util::Array<Ptr<ToolkitUtil::SkinFragment> >& fragments);
    /// returns material
    const Util::String& GetMaterial() const;    

    /// sets mesh index
    void SetGroupId(const IndexT& id);
    /// gets mesh group id
    const IndexT& GetGroupId() const;

    /// returns true if node has lod group
    const bool HasLOD() const;
    /// returns max distance
    const float GetLODMaxDistance() const;
    /// returns min distance
    const float GetLODMinDistance() const;

    /// extracts mesh data
    void ExtractMesh();
protected:

    /// helper function for extracting UVs
    void ExtractUVs(int polygonVertex, int vertexIndex, int uvLayer, ToolkitUtil::MeshBuilderVertex& vertex);
    /// helper function for extracting normals
    void ExtractNormals(int polygonVertex, int vertexIndex, ToolkitUtil::MeshBuilderVertex& vertex);
    /// helper function for extracting normals
    void ExtractBinormalsAndTangents(int polygonVertex, int vertexIndex, ToolkitUtil::MeshBuilderVertex& vertex);
    /// helper function for extracting vertex colors
    void ExtractColors(int polygonVertex, int vertexIndex, ToolkitUtil::MeshBuilderVertex& vertex);
    /// extracts skin data
    void ExtractSkin();
    /// helper function for calculating normals
    void CalculateNormals();
    /// helper function for calculating tangents and binormals
    void CalculateTangentsAndBinormals();
    /// generates rigid weights for parented joints
    void GenerateRigidSkin();

    /// merges children
    void DoMerge(Util::Dictionary<Util::String, Util::Array<Ptr<NFbxMeshNode> > >& meshes);

    ToolkitUtil::MeshFlags                              meshFlags;
    ToolkitUtil::ExportFlags                            exportFlags;
    ToolkitUtil::ExportMode                             exportMode;

    FbxMesh*                                            fbxMesh;
    FbxLODGroup*                                        lod;
    IndexT                                              lodIndex;
    ToolkitUtil::MeshBuilder*                           mesh;

    Ptr<NFbxJointNode>                                  skeletonLink;
    Util::Array<Ptr<ToolkitUtil::SkinFragment> >        skinFragments;

    IndexT                                              groupId;
    Util::String                                        material;
    Math::bbox                                          boundingBox;
}; 

//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxMeshNode::SetExportFlags( const ToolkitUtil::ExportFlags& flags )
{
    this->exportFlags = flags;
}

//------------------------------------------------------------------------------
/**
*/
inline const ToolkitUtil::ExportFlags& 
NFbxMeshNode::GetExportFlags() const
{
    return this->exportFlags;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxMeshNode::SetExportMode( const ToolkitUtil::ExportMode& mode )
{
    this->exportMode = mode;
}

//------------------------------------------------------------------------------
/**
*/
inline const ToolkitUtil::ExportMode& 
NFbxMeshNode::GetExportMode() const
{
    return this->exportMode;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxMeshNode::SetBoundingBox( const Math::bbox& bbox )
{
    this->boundingBox = bbox;
}

//------------------------------------------------------------------------------
/**
*/
inline const Math::bbox& 
NFbxMeshNode::GetBoundingBox() const
{
    return this->boundingBox;
}

//------------------------------------------------------------------------------
/**
*/
inline const ToolkitUtil::MeshFlags&
NFbxMeshNode::GetMeshFlags() const
{
    return this->meshFlags;
}


//------------------------------------------------------------------------------
/**
*/
inline Ptr<NFbxJointNode> 
NFbxMeshNode::GetSkeletonLink() const
{
    return this->skeletonLink;
}


//------------------------------------------------------------------------------
/**
*/
inline ToolkitUtil::MeshBuilder* 
NFbxMeshNode::GetMesh() const
{
    return this->mesh;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxMeshNode::SetMesh( ToolkitUtil::MeshBuilder* newMesh )
{
    this->mesh = newMesh;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::Array<Ptr<ToolkitUtil::SkinFragment> >& 
NFbxMeshNode::GetSkinFragments() const
{
    return this->skinFragments;
}


//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxMeshNode::SetSkinFragments( const Util::Array<Ptr<ToolkitUtil::SkinFragment> >& fragments )
{
    this->skinFragments = fragments;
}

//------------------------------------------------------------------------------
/**
*/
inline const Util::String& 
NFbxMeshNode::GetMaterial() const
{
    return this->material;
}

//------------------------------------------------------------------------------
/**
*/
inline void 
NFbxMeshNode::SetGroupId( const IndexT& id )
{
    this->groupId = id;
}

//------------------------------------------------------------------------------
/**
*/
inline const IndexT& 
NFbxMeshNode::GetGroupId() const
{
    return this->groupId;
}

//------------------------------------------------------------------------------
/**
*/
inline const bool 
NFbxMeshNode::HasLOD() const
{
    return this->lod != NULL;
}


} // namespace ToolkitUtil
//------------------------------------------------------------------------------