#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxMeshNode
    
    Wraps an FBX mesh node into a Nebula-style fbx node
    
    (C) 2012-2016 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "toolkit-common/base/exporttypes.h"
#include "nfbxnode.h"
#include "model/import/base/scenenode.h"
#include "math/vec2.h"
#include "math/vec4.h"
#include "ufbx/ufbx.h"

extern uint MeshCounter;
namespace ToolkitUtil
{

class SceneNode;
class NFbxMeshNode
{
public:

    typedef uint MeshMask;

    /// Setup node from FBX node
    static void Setup(SceneNode* node, SceneNode* parent, ufbx_node* fbxNode);

    /// Extract mesh
    static void ExtractMesh(
        SceneNode* node
        , Util::Array<MeshBuilder>& meshes
        , const Util::Dictionary<ufbx_node*, SceneNode*>& nodeLookup
        , const ToolkitUtil::ExportFlags flags
    );

protected:
    friend class NFbxScene;


    /// Helper to extract UV data
    static Math::vec2 Extract(const ufbx_vertex_vec2* data, int polygonVertex);
    /// Helper to extract color data
    static Math::vec4 Extract(const ufbx_vertex_vec4* data, int polygonVertex);
    /// Helper to extract generic 4 float vector data
    static Math::vec4 Extract(const ufbx_vertex_vec3* data, int polygonVertex);
    /// extracts skin data
    static void ExtractSkin(SceneNode* node, Util::FixedArray<Math::uint4>& indices, Util::FixedArray<Math::vec4>& weights, const Util::Dictionary<ufbx_node*, SceneNode*>& nodeLookup, ufbx_mesh* fbxMesh);
    /// generates rigid weights for parented joints
    static void GenerateRigidSkin(SceneNode* node, Util::FixedArray<Math::uint4>& indices, Util::FixedArray<Math::vec4>& weights, uint& meshFlags);
}; 


//------------------------------------------------------------------------------
/**
*/
inline Math::vec2
NFbxMeshNode::Extract(const ufbx_vertex_vec2* data, int polygonVertex)
{
    ufbx_vec2 n = data->values.data[data->indices.data[polygonVertex]];
    return FbxToMath(n);
}

//------------------------------------------------------------------------------
/**
*/
inline Math::vec4
NFbxMeshNode::Extract(const ufbx_vertex_vec4* data, int polygonVertex)
{
    ufbx_vec4 n = data->values.data[data->indices.data[polygonVertex]];
    return Math::vec4(n.x, n.y, n.z, n.w);
}

//------------------------------------------------------------------------------
/**
*/
inline Math::vec4
NFbxMeshNode::Extract(const ufbx_vertex_vec3* data, int polygonVertex)
{
    ufbx_vec3 n = data->values.data[data->indices.data[polygonVertex]];
    return FbxToMath(n);
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------