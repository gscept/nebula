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
#include <fbxsdk.h>

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
    static Math::vec2 Extract(const fbxsdk::FbxLayerElementTemplate<fbxsdk::FbxVector2>* data, int polygonVertex, int vertexIndex);
    /// Helper to extract color data
    static Math::vec4 Extract(const fbxsdk::FbxLayerElementTemplate<fbxsdk::FbxColor>* data, int polygonVertex, int vertexIndex);
    /// Helper to extract generic 4 float vector data
    static Math::vec4 Extract(const fbxsdk::FbxLayerElementTemplate<fbxsdk::FbxVector4>* data, int polygonVertex, int vertexIndex);
    /// extracts skin data
    static void ExtractSkin(SceneNode* node, Util::FixedArray<Math::uint4>& indices, Util::FixedArray<Math::vec4>& weights, const Util::Dictionary<fbxsdk::FbxNode*, SceneNode*>& nodeLookup, fbxsdk::FbxMesh* fbxMesh);
    /// generates rigid weights for parented joints
    static void GenerateRigidSkin(SceneNode* node, Util::FixedArray<Math::uint4>& indices, Util::FixedArray<Math::vec4>& weights, uint& meshFlags);
}; 


//------------------------------------------------------------------------------
/**
*/
inline Math::vec2
NFbxMeshNode::Extract(const fbxsdk::FbxLayerElementTemplate<fbxsdk::FbxVector2>* data, int polygonVertex, int vertexIndex)
{
    FbxVector2 n;
    switch (data->GetMappingMode())
    {
        case fbxsdk::FbxLayerElement::eByPolygonVertex:
            switch (data->GetReferenceMode())
            {
                case fbxsdk::FbxLayerElement::eDirect:
                    n = data->GetDirectArray().GetAt(vertexIndex);
                    break;
                case fbxsdk::FbxLayerElement::eIndexToDirect:
                    n = data->GetDirectArray().GetAt(data->GetIndexArray().GetAt(vertexIndex));
                    break;
            }
            break;
        case fbxsdk::FbxLayerElement::eByControlPoint:
            switch (data->GetReferenceMode())
            {
                case fbxsdk::FbxLayerElement::eDirect:
                    n = data->GetDirectArray().GetAt(polygonVertex);
                    break;
                case fbxsdk::FbxLayerElement::eIndexToDirect:
                    n = data->GetDirectArray().GetAt(data->GetIndexArray().GetAt(polygonVertex));
                    break;
            }
            break;
    }
    n.FixIncorrectValue();
    return FbxToMath(n);
}

//------------------------------------------------------------------------------
/**
*/
inline Math::vec4
NFbxMeshNode::Extract(const fbxsdk::FbxLayerElementTemplate<fbxsdk::FbxColor>* data, int polygonVertex, int vertexIndex)
{
    FbxColor n;
    switch (data->GetMappingMode())
    {
        case fbxsdk::FbxLayerElement::eByPolygonVertex:
            switch (data->GetReferenceMode())
            {
                case fbxsdk::FbxLayerElement::eDirect:
                    n = data->GetDirectArray().GetAt(vertexIndex);
                    break;
                case fbxsdk::FbxLayerElement::eIndexToDirect:
                    n = data->GetDirectArray().GetAt(data->GetIndexArray().GetAt(vertexIndex));
                    break;
            }
            break;
        case fbxsdk::FbxLayerElement::eByControlPoint:
            switch (data->GetReferenceMode())
            {
                case fbxsdk::FbxLayerElement::eDirect:
                    n = data->GetDirectArray().GetAt(polygonVertex);
                    break;
                case fbxsdk::FbxLayerElement::eIndexToDirect:
                    n = data->GetDirectArray().GetAt(data->GetIndexArray().GetAt(polygonVertex));
                    break;
            }
            break;
    }
    return Math::vec4(n[0], n[1], n[2], n[3]);
}

//------------------------------------------------------------------------------
/**
*/
inline Math::vec4
NFbxMeshNode::Extract(const fbxsdk::FbxLayerElementTemplate<fbxsdk::FbxVector4>* data, int polygonVertex, int vertexIndex)
{
    FbxVector4 n;
    switch (data->GetMappingMode())
    {
        case fbxsdk::FbxLayerElement::eByPolygonVertex:
            switch (data->GetReferenceMode())
            {
                case fbxsdk::FbxLayerElement::eDirect:
                    n = data->GetDirectArray().GetAt(vertexIndex);
                    break;
                case fbxsdk::FbxLayerElement::eIndexToDirect:
                    n = data->GetDirectArray().GetAt(data->GetIndexArray().GetAt(vertexIndex));
                    break;
            }
            break;
        case fbxsdk::FbxLayerElement::eByControlPoint:
            switch (data->GetReferenceMode())
            {
                case fbxsdk::FbxLayerElement::eDirect:
                    n = data->GetDirectArray().GetAt(polygonVertex);
                    break;
                case fbxsdk::FbxLayerElement::eIndexToDirect:
                    n = data->GetDirectArray().GetAt(data->GetIndexArray().GetAt(polygonVertex));
                    break;
            }
            break;
    }
    n.FixIncorrectValue();
    return FbxToMath(n);
}

} // namespace ToolkitUtil
//------------------------------------------------------------------------------