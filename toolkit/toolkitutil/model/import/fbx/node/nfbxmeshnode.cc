//------------------------------------------------------------------------------
//  fbxmeshnode.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "nfbxnode.h"
#include "nfbxmeshnode.h"
#include "model/meshutil/meshbuildervertex.h"
#include "nfbxscene.h"

namespace ToolkitUtil
{

using namespace Math;
using namespace Util;
using namespace CoreAnimation;
using namespace ToolkitUtil;

uint MeshCounter = 0;

//------------------------------------------------------------------------------
/**
*/
void 
NFbxMeshNode::Setup(
    SceneNode* node
    , SceneNode* parent
    , fbxsdk::FbxNode* fbxNode
)
{
    NFbxNode::Setup(node, parent, fbxNode);
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxMeshNode::ExtractMesh(
    SceneNode* node
    , Util::Array<MeshBuilder>& meshes
    , const Util::Dictionary<fbxsdk::FbxNode*, SceneNode*>& nodeLookup
    , const ToolkitUtil::ExportFlags flags
)
{
    node->mesh.meshIndex = meshes.Size();

    // Add a single mesh primitive for FBX nodes
    MeshBuilder& mesh = meshes.Emplace();

    if (node->fbx.node->GetMaterialCount())
        node->mesh.material = node->fbx.node->GetMaterial(0)->GetName();

    FbxMesh* fbxMesh = node->fbx.node->GetMesh();
    if (!fbxMesh->IsTriangleMesh())
        n_error("Node '%s' -> Mesh '%s' is not a triangle mesh. Make sure your exported meshes are triangulated!\n", node->fbx.node->GetName(), fbxMesh->GetName());

    // create mask
    uint meshMask = ToolkitUtil::NoMeshFlags;
    if (fbxMesh->GetDeformerCount(FbxDeformer::eSkin) > 0)
    {
        meshMask |= ToolkitUtil::HasSkin;
    }

    // tag as physics if parent name is physics, note that this will only work of the direct parent is called physics!
    SceneNode* parent = node->base.parent;
    if (parent != nullptr && parent->base.isPhysics)
    {
        node->mesh.material = "physics";
    }

    if (fbxMesh->GetUVLayerCount() > 1)
        meshMask |= HasMultipleUVs;

    if (fbxMesh->GetElementVertexColorCount() > 0)
        meshMask |= HasVertexColors;

    if (fbxMesh->GetElementTangentCount() > 0)
        meshMask |= HasTangents;

    if (fbxMesh->GetElementNormalCount() > 0)
        meshMask |= HasNormals;

    // Provoke generation of LOD groups
    FbxLODGroup* lodGroup = nullptr;
    if (node->fbx.node->GetParent() != nullptr)
    {
        lodGroup = node->fbx.node->GetParent()->GetLodGroup();
        if (lodGroup != nullptr)
        {
            // DO NOT REMOVE THIS. IF YOU DO, YOU DON'T GET THE LODS PROPERLY.
            // ALSO CALLED A FUCKING BUG.
            int numThresholds = lodGroup->GetNumThresholds();
            int displayLevels = lodGroup->GetNumDisplayLevels();
        }
    }

    if (AllBits(meshMask, HasNormals))
    {
        bool res = fbxMesh->GenerateTangentsData(0, true);
        n_assert(res);
        meshMask |= HasTangents;
    }

    // set mask
    node->mesh.meshFlags = (ToolkitUtil::MeshFlags)meshMask;

    // Proceed to extract mesh
    int vertexCount = fbxMesh->GetControlPointsCount();
    int polyCount = fbxMesh->GetPolygonCount();
    int uvCount = fbxMesh->GetUVLayerCount();
    int normalCount = fbxMesh->GetElementNormalCount();
    int tangentCount = fbxMesh->GetElementTangentCount();
    int colorCount = fbxMesh->GetElementVertexColorCount();

    // this is here just to inform if an artist has forgot to apply a UV set or the mesh has no normals prior to importing
    if (vertexCount > 0)
    {
        n_assert2(uvCount > 0, "You need at least one UV-channel or no shader will be applicable!");
        n_assert2(normalCount > 0, "You need at least one set of normals or no shader will be applicable!");
    }

    // get scale
    uint groupId = MeshCounter++;
    node->mesh.groupId = groupId;

    MeshBuilderVertex::ComponentMask componentMask = 0x0;
    Util::FixedArray<Math::vec4> controlPoints;
    controlPoints.Resize(fbxMesh->GetControlPointsCount());

    mesh.NewMesh(controlPoints.Size(), controlPoints.Size() * 3);
    mesh.SetPrimitiveTopology(CoreGraphics::PrimitiveTopology::TriangleList);
    for (IndexT i = 0; i < controlPoints.Size(); i++)
    {
        FbxVector4 v = fbxMesh->GetControlPointAt(i);
        v.FixIncorrectValue();
        controlPoints[i] = FbxToMath(v) * AdjustedScale;
        componentMask |= MeshBuilderVertex::Position;
    }

    Util::FixedArray<Math::vec4> skinWeights;
    Util::FixedArray<Math::uint4> skinIndices;

    // if we export using a skeletal model, we always need a skin, so if the mesh isn't skinned, we do it manually
    if (node->mesh.meshFlags & HasSkin)
    {
        // extract skin
        skinWeights.Resize(controlPoints.Size());
        skinIndices.Resize(controlPoints.Size());
        ExtractSkin(node, skinIndices, skinWeights, nodeLookup, fbxMesh);
        componentMask |= MeshBuilderVertex::SkinWeights | MeshBuilderVertex::SkinIndices;
    }
    else if (flags & ToolkitUtil::CalcRigidSkin)
    { 
        // generate rigid skin if necessary
        skinWeights.Resize(controlPoints.Size());
        skinIndices.Resize(controlPoints.Size());
        GenerateRigidSkin(node, skinIndices, skinWeights, (uint&)node->mesh.meshFlags);
        componentMask |= MeshBuilderVertex::SkinWeights | MeshBuilderVertex::SkinIndices;
    }

    typedef fbxsdk::FbxLayerElementTemplate<fbxsdk::FbxVector4>* FbxVec4Array;
    typedef fbxsdk::FbxLayerElementTemplate<fbxsdk::FbxVector2>* FbxVec2Array;
    typedef fbxsdk::FbxLayerElementTemplate<fbxsdk::FbxColor>* FbxColorArray;
    typedef fbxsdk::FbxLayerElementTemplate<int>* FbxIndexArray;

    FbxVec2Array uv0Elements = nullptr, uv1Elements = nullptr;
    FbxColorArray colorElements = nullptr;
    FbxVec4Array normalElements = nullptr, tangentElements = nullptr;

    // Convert all meshes to map their indexes to once value per vertex
    if (uvCount > 0)
        uv0Elements = fbxMesh->GetElementUV(0);
    if (uvCount > 1)
        uv1Elements = fbxMesh->GetElementUV(1);
    if (colorCount > 0)
        colorElements = fbxMesh->GetElementVertexColor(0);
    if (normalCount > 0)
        normalElements = fbxMesh->GetElementNormal(0);
    if (tangentCount > 0)
        tangentElements = fbxMesh->GetElementTangent(0);

    // setup triangles
    int vertexId = 0;
    for (int polygonIndex = 0; polygonIndex < polyCount; polygonIndex++)
    {
        int polygonSize = fbxMesh->GetPolygonSize(polygonIndex);
        n_assert2(polygonSize == 3, "Some polygons seem to not be triangulated, this is not accepted");
        MeshBuilderTriangle meshTriangle;
        for (int polygonVertexIndex = 0; polygonVertexIndex < polygonSize; polygonVertexIndex++)
        {
            // we want to offset the vertex index with the current size of the mesh
            int polygonVertex = fbxMesh->GetPolygonVertex(polygonIndex, polygonVertexIndex);
            MeshBuilderVertex meshVertex;
            meshVertex.SetPosition(controlPoints[polygonVertex]);

            // extract uvs from the 0th channel
            Math::vec2 uv = Extract(uv0Elements, polygonVertex, vertexId);
            meshVertex.SetUv(uv);
            componentMask |= MeshBuilderVertex::Uvs;

            if (AllBits(componentMask, MeshBuilderVertex::SkinWeights | MeshBuilderVertex::SkinIndices))
            {
                meshVertex.SetSkinWeights(skinWeights[polygonVertex]);
                meshVertex.SetSkinIndices(skinIndices[polygonVertex]);
            }

            // extract multilayered uvs, or ordinary uvs if we're not importing as multilayered
            if (AllBits(flags, ToolkitUtil::ImportSecondaryUVs))
            {
                if (AllBits(node->mesh.meshFlags, HasMultipleUVs))
                {
                    // simply extract two uvs
                    Math::vec2 uv2 = Extract(uv1Elements, polygonVertex, vertexId);
                    meshVertex.SetSecondaryUv(uv2);
                }
                else
                {
                    // if mesh has none, flood to 0
                    meshVertex.SetSecondaryUv(Math::vec2(0, 0));
                }
                componentMask |= MeshBuilderVertex::SecondUv;
            }

            // extract colors if so requested
            if (AllBits(flags, ToolkitUtil::ImportColors))
            {
                if (AllBits(node->mesh.meshFlags, HasVertexColors))
                {
                    // extract colors
                    Math::vec4 color = Extract(colorElements, polygonVertex, vertexId);
                    meshVertex.SetColor(color);
                }
                else
                {
                    // set vertex color to be white if no vertex colors can be extracted from FBX
                    meshVertex.SetColor(Math::vec4(1));
                }
                componentMask |= MeshBuilderVertex::Color;
            }

            Math::vec4 normal = Extract(normalElements, polygonVertex, vertexId);
            meshVertex.SetNormal(xyz(normal));
            componentMask |= MeshBuilderVertex::Normals;

            if (AllBits(node->mesh.meshFlags, HasTangents))
            {
                Math::vec4 tangent = Extract(tangentElements, polygonVertex, vertexId);
                meshVertex.SetTangent(xyz(tangent));
                meshVertex.SetSign(-tangent.w);
                componentMask |= MeshBuilderVertex::Tangents;
            }
            else
            {
                meshVertex.SetTangent({ 0, 0, 0 });
                meshVertex.SetSign(0);
            }

            mesh.AddVertex(meshVertex);
            meshTriangle.SetVertexIndex(polygonVertexIndex, vertexId);

            vertexId++;
        }
        mesh.AddTriangle(meshTriangle);
    }


    // flip uvs if checked
    if (AllBits(flags, ToolkitUtil::FlipUVs))
    {
        mesh.FlipUvs();
    }

    // compute boundingbox
    node->base.boundingBox = mesh.ComputeBoundingBox();
    mesh.SetComponents(componentMask);

    // remove redundant vertices
    mesh.Cleanup(nullptr);

    // Calculate lods
    if (lodGroup != nullptr)
    {
        node->mesh.lodIndex = node->base.parent->base.children.FindIndex(node);
        FbxDistance dist;
        if (lodGroup->GetThreshold(node->mesh.lodIndex - 1, dist))
            node->mesh.minLodDistance = dist.value() * SceneScale;
        else
            node->mesh.minLodDistance = 0;

        if (lodGroup->GetThreshold(node->mesh.lodIndex, dist))
            node->mesh.maxLodDistance = dist.value() * SceneScale;
        else
            node->mesh.maxLodDistance = 0;
    }

    // clear up FBX mesh
    fbxMesh->Destroy();
}

//------------------------------------------------------------------------------
/**
*/
FbxAMatrix 
GetGeometryTransformation(FbxNode* inNode)
{
    if (!inNode)
    {
        throw std::exception("Null for mesh geometry");
    }

    const FbxVector4 lT = inNode->GetGeometricTranslation(FbxNode::eSourcePivot);
    const FbxVector4 lR = inNode->GetGeometricRotation(FbxNode::eSourcePivot);
    const FbxVector4 lS = inNode->GetGeometricScaling(FbxNode::eSourcePivot);

    return FbxAMatrix(lT, lR, lS);
}

//------------------------------------------------------------------------------
/**
    Hmm, maybe a way to check if we have more than 255 indices, then we should switch to using Joint indices as complete uints?
*/
void 
NFbxMeshNode::ExtractSkin(SceneNode* node, Util::FixedArray<Math::uint4>& indices, Util::FixedArray<Math::vec4>& weights, const Util::Dictionary<fbxsdk::FbxNode*, SceneNode*>& nodeLookup, fbxsdk::FbxMesh* fbxMesh)
{
    int vertexCount = fbxMesh->GetControlPointsCount();
    int skinCount = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
    
    Util::FixedArray<Util::Array<std::tuple<int, float>>> keyWeightPairs(vertexCount);
    FbxAMatrix geometricTransform = GetGeometryTransformation(node->fbx.node);
    for (int skinIndex = 0; skinIndex < skinCount; skinIndex++)
    {
        FbxSkin* skin = static_cast<FbxSkin*>(fbxMesh->GetDeformer(skinIndex, FbxDeformer::eSkin));
        int clusterCount = skin->GetClusterCount();
        node->base.isSkin = true;

        for (int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++)
        {
            FbxCluster* cluster = skin->GetCluster(clusterIndex);
            FbxNode* joint = cluster->GetLink();
            SceneNode* jointNode = nodeLookup[joint];
            n_assert(jointNode != nullptr);
            n_assert(jointNode->skeleton.bindMatrix == Math::mat4());
            n_assert(jointNode->type == SceneNode::NodeType::Joint);

            // Calculate inverse transform for skinned vertices
            FbxAMatrix linkMatrix, transformMatrix;
            cluster->GetTransformLinkMatrix(linkMatrix);
            cluster->GetTransformMatrix(transformMatrix);
            FbxAMatrix inversedPose = linkMatrix.Inverse();
            inversedPose = inversedPose * transformMatrix * geometricTransform;
            jointNode->skeleton.bindMatrix = FbxToMath(inversedPose);
            jointNode->skeleton.bindMatrix.position *= AdjustedScale;


            int clusterVertexIndexCount = cluster->GetControlPointIndicesCount();
            for (int vertexIndex = 0; vertexIndex < clusterVertexIndexCount; vertexIndex++)
            {
                int vertex = cluster->GetControlPointIndices()[vertexIndex];
                float weight = cluster->GetControlPointWeights()[vertexIndex];
                if (weight > 0)
                    keyWeightPairs[vertex].Append(std::make_tuple(jointNode->skeleton.jointIndex, weight));
            }
        }
    }

    // finally, traverse through every vertex and set their joint indices and weighs
    for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
    {
        Util::Array<std::tuple<int, float>>& weightPairs = keyWeightPairs[vertexIndex];
        weightPairs.SortWithFunc([](const std::tuple<int, float>& lhs, const std::tuple<int, float>& rhs)->bool
        {
            const auto& [i0, w0] = lhs;
            const auto& [i1, w1] = rhs;
            return w0 > w1;
        });

        const auto& [i0, w0] = weightPairs.Size() > 0 ? weightPairs[0] : std::make_tuple(0, 0.0f);
        const auto& [i1, w1] = weightPairs.Size() > 1 ? weightPairs[1] : std::make_tuple(0, 0.0f);
        const auto& [i2, w2] = weightPairs.Size() > 2 ? weightPairs[2] : std::make_tuple(0, 0.0f);
        const auto& [i3, w3] = weightPairs.Size() > 3 ? weightPairs[3] : std::make_tuple(0, 0.0f);

        float normalizeFactor = 1.0f / (w0 + w1 + w2 + w3);

        // set weights
        weights[vertexIndex] = vec4(w0 * normalizeFactor, w1 * normalizeFactor, w2 * normalizeFactor, w3 * normalizeFactor);
        indices[vertexIndex] = uint4{ (uint)i0, (uint)i1, (uint)i2, (uint)i3 };
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxMeshNode::GenerateRigidSkin(SceneNode* node, Util::FixedArray<Math::uint4>& indices, Util::FixedArray<Math::vec4>& weights, uint& meshFlags)
{
    if (!AllBits(meshFlags, ToolkitUtil::HasSkin))
    {
        // parent to joint if it exists, otherwise just parent to root joint
        SceneNode* parent = node->base.parent;
        if (parent != nullptr && parent->type == SceneNode::NodeType::Joint)
        {
            IndexT parentJointIndex = parent->skeleton.jointIndex;

            int vertCount = indices.Size();
            IndexT vertIndex;
            for (vertIndex = 0; vertIndex < vertCount; vertIndex++)
            {
                // for each vertex, set a rigid skin by using a weight of 1 and the index of the parent node
                indices[vertIndex] = uint4{ (uint)parentJointIndex, 0, 0, 0 };
                weights[vertIndex] = vec4(1, 0, 0, 0);
            }
        }
        else
        {
            int vertCount = indices.Size();
            IndexT vertIndex;
            for (vertIndex = 0; vertIndex < vertCount; vertIndex++)
            {
                // for each vertex, set a rigid skin by using a weight of 1 and the index of the parent node
                indices[vertIndex] = uint4{ 0, 0, 0, 0 };
                weights[vertIndex] = vec4(1, 0, 0, 0);
            }
        }
        
        // set skin flag
        meshFlags |= ToolkitUtil::HasSkin;
    }
}

} // namespace ToolkitUtil