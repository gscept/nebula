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
    , Util::Array<MeshBuilder>& meshes
    , const Util::Dictionary<fbxsdk::FbxNode*, SceneNode*>& nodeLookup
    , fbxsdk::FbxNode* fbxNode
    , const ToolkitUtil::ExportFlags flags
)
{
    NFbxNode::Setup(node, fbxNode);
    FbxMesh* fbxMesh = fbxNode->GetMesh();

    node->mesh.meshIndex = meshes.Size();
    

    // Add a single mesh primitive for FBX nodes
    MeshBuilder& mesh = meshes.Emplace();

    if (fbxNode->GetMaterialCount())
        node->mesh.material = fbxNode->GetMaterial(0)->GetName();

    if (!fbxMesh->IsTriangleMesh())
        n_error("Node '%s' -> Mesh '%s' is not a triangle mesh. Make sure your exported meshes are triangulated!\n", fbxNode->GetName(), fbxMesh->GetName());

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
    if (fbxNode->GetParent() != nullptr)
    {
        lodGroup = fbxNode->GetParent()->GetLodGroup();
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
    float scaleFactor = AdjustedScale;
    uint groupId = MeshCounter++;
    node->mesh.groupId = groupId;

    MeshBuilderVertex::ComponentMask componentMask = 0x0;

    // setup vertices with basic per-vertex data
    for (int vertex = 0; vertex < vertexCount; vertex++)
    {
        FbxVector4 v = fbxMesh->GetControlPointAt(vertex);
        MeshBuilderVertex meshVertex;
        vec4 position = vec4((float)v[0], (float)v[1], (float)v[2], 1.0f) * vec4(scaleFactor, scaleFactor, scaleFactor, 1.0f);
        componentMask |= MeshBuilderVertex::Position;
        meshVertex.SetPosition(position);
        mesh.AddVertex(meshVertex);
    }

    // setup triangles
    for (int polygonIndex = 0; polygonIndex < polyCount; polygonIndex++)
    {
        int polygonSize = fbxMesh->GetPolygonSize(polygonIndex);
        n_assert2(polygonSize == 3, "Some polygons seem to not be triangulated, this is not accepted");
        MeshBuilderTriangle meshTriangle;
        for (int polygonVertexIndex = 0; polygonVertexIndex < polygonSize; polygonVertexIndex++)
        {
            // we want to offset the vertex index with the current size of the mesh
            int polygonVertex = fbxMesh->GetPolygonVertex(polygonIndex, polygonVertexIndex);
            meshTriangle.SetVertexIndex(polygonVertexIndex, polygonVertex);
        }
        mesh.AddTriangle(meshTriangle);
    }


    // if we export using a skeletal model, we always need a skin, so if the mesh isn't skinned, we do it manually
    if (node->mesh.meshFlags & HasSkin)
    {
        // extract skin
        ExtractSkin(node, mesh, nodeLookup, fbxMesh);
        componentMask |= MeshBuilderVertex::SkinWeights | MeshBuilderVertex::SkinIndices;
    }
    else if (flags & ToolkitUtil::CalcRigidSkin)
    {
        // generate rigid skin if necessary
        GenerateRigidSkin(node, mesh, (uint&)node->mesh.meshFlags);
        componentMask |= MeshBuilderVertex::SkinWeights | MeshBuilderVertex::SkinIndices;
    }

    mesh.Inflate();

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

    int vertexId = 0;
    for (int triangleIndex = 0; triangleIndex < polyCount; triangleIndex++)
    {
        MeshBuilderTriangle& triangle = mesh.TriangleAt(triangleIndex);
        for (int polygonVertexIndex = 0; polygonVertexIndex < 3; polygonVertexIndex++)
        {
            int polygonVertex = fbxMesh->GetPolygonVertex(triangleIndex, polygonVertexIndex);
            int vertexIndex = triangle.GetVertexIndex(polygonVertexIndex);
            MeshBuilderVertex& vertexRef = mesh.VertexAt(vertexIndex);

            // extract uvs from the 0th channel
            Math::vec2 uv = Extract(uv0Elements, polygonVertex, vertexId);
            vertexRef.SetUv(uv);
            componentMask |= MeshBuilderVertex::Uvs;

            // extract multilayered uvs, or ordinary uvs if we're not importing as multilayered
            if (AllBits(flags, ToolkitUtil::ImportSecondaryUVs))
            {
                if (AllBits(node->mesh.meshFlags, HasMultipleUVs))
                {
                    // simply extract two uvs
                    Math::vec2 uv2 = Extract(uv1Elements, polygonVertex, vertexId);
                    vertexRef.SetSecondaryUv(uv2);
                }
                else
                {
                    // if mesh has none, flood to 0
                    vertexRef.SetSecondaryUv(Math::vec2(0, 0));
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
                    vertexRef.SetColor(color);
                }
                else
                {
                    // set vertex color to be white if no vertex colors can be extracted from FBX
                    vertexRef.SetColor(Math::vec4(1));
                }
                componentMask |= MeshBuilderVertex::Color;
            }

            Math::vec4 normal = Extract(normalElements, polygonVertex, vertexId);
            vertexRef.SetNormal(xyz(normal));
            componentMask |= MeshBuilderVertex::Normals;

            if (AllBits(node->mesh.meshFlags, HasTangents))
            {
                Math::vec4 tangent = Extract(tangentElements, polygonVertex, vertexId);
                vertexRef.SetTangent(xyz(tangent));
                vertexRef.SetSign(-tangent.w);
                componentMask |= MeshBuilderVertex::Tangents;
            }
            else
            {
                vertexRef.SetTangent({ 0, 0, 0 });
                vertexRef.SetSign(0);
            }

            vertexId++;
        }
    }

    // flip uvs if checked
    if (AllBits(flags, ToolkitUtil::FlipUVs))
    {
        mesh.FlipUvs();
    }

    // compute boundingbox
    node->base.boundingBox = mesh.ComputeBoundingBox();
    mesh.SetComponents(componentMask);

    Util::FixedArray<Util::Array<int>> collapseMap;
    mesh.Deflate(&collapseMap);

    // deflate to remove redundant vertices if flag is set
    if (AllBits(flags, ToolkitUtil::RemoveRedundant))
    {
        // remove redundant vertices
        mesh.Cleanup(nullptr);
    }

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
    Hmm, maybe a way to check if we have more than 255 indices, then we should switch to using Joint indices as complete uints?
*/
void 
NFbxMeshNode::ExtractSkin(SceneNode* node, MeshBuilder& mesh, const Util::Dictionary<fbxsdk::FbxNode*, SceneNode*>& nodeLookup, fbxsdk::FbxMesh* fbxMesh)
{
    int vertexCount = fbxMesh->GetControlPointsCount();
    int skinCount = fbxMesh->GetDeformerCount(FbxDeformer::eSkin);
    int* jointArray = new int[vertexCount*4];
    double* weightArray = new double[vertexCount*4];
    int* slotArray = new int[vertexCount];
    memset(jointArray, 0, sizeof(int)*vertexCount*4);
    memset(weightArray, 0, sizeof(double)*vertexCount*4);
    memset(slotArray, 0, sizeof(int)*vertexCount);
    int maxIndex = 0;

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
            if (jointNode->type == SceneNode::NodeType::Joint)
            {
                int clusterVertexIndexCount = cluster->GetControlPointIndicesCount();
                for (int vertexIndex = 0; vertexIndex < clusterVertexIndexCount; vertexIndex++)
                {
                    int vertex = cluster->GetControlPointIndices()[vertexIndex];
                    double weight = cluster->GetControlPointWeights()[vertexIndex];
                    int stride = slotArray[vertex];

                    // this is just a fail safe for smooth operators
                    if (vertex >= vertexCount)
                        continue;

                    int* jointData = jointArray + (vertex*4);
                    double* weightData = weightArray + (vertex*4);

                    // ignore weights and indices over 4 (optimal vertex-to-joint ratio for games is 4)
                    if (stride > 3) continue;

                    jointData[stride] = jointNode->skeleton.jointIndex;
                    weightData[stride] = weight;
                    maxIndex = Math::max(jointData[stride], maxIndex);
                    slotArray[vertex]++;
                }
            }
        }
    }

    // finally, traverse through every vertex and set their joint indices and weighs
    for (int vertexIndex = 0; vertexIndex < vertexCount; vertexIndex++)
    {
        MeshBuilderVertex& vertex = mesh.VertexAt(vertexIndex);
        int* indices = jointArray + (vertexIndex*4);
        double* weights = weightArray + (vertexIndex*4);

        // set weights
        vertex.SetSkinWeights(vec4(weights[0], weights[1], weights[2], weights[3]));
        vertex.SetSkinIndices(uint4{ (uint)indices[0], (uint)indices[1], (uint)indices[2], (uint)indices[3] });
    }

    delete [] slotArray;
    delete [] jointArray;
    delete [] weightArray;
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxMeshNode::GenerateRigidSkin(SceneNode* node, MeshBuilder& mesh, uint& meshFlags)
{
    if (!AllBits(meshFlags, ToolkitUtil::HasSkin))
    {
        // parent to joint if it exists, otherwise just parent to root joint
        SceneNode* parent = node->base.parent;
        if (parent != nullptr && parent->type == SceneNode::NodeType::Joint)
        {
            IndexT parentJointIndex = parent->skeleton.jointIndex;

            int vertCount = mesh.GetNumVertices();
            IndexT vertIndex;
            for (vertIndex = 0; vertIndex < vertCount; vertIndex++)
            {
                // for each vertex, set a rigid skin by using a weight of 1 and the index of the parent node
                MeshBuilderVertex& vertex = mesh.VertexAt(vertIndex);
                vertex.SetSkinWeights(vec4(1, 0, 0, 0));
                vertex.SetSkinIndices(uint4{ (uint)parentJointIndex, 0, 0, 0 });
            }
        }
        else
        {
            int vertCount = mesh.GetNumVertices();
            IndexT vertIndex;
            for (vertIndex = 0; vertIndex < vertCount; vertIndex++)
            {
                // for each vertex, set a rigid skin by using a weight of 1 and the index of the parent node
                MeshBuilderVertex& vertex = mesh.VertexAt(vertIndex);
                vertex.SetSkinWeights(vec4(1, 0, 0, 0));
                vertex.SetSkinIndices(uint4{ 0, 0, 0, 0 });
            }
        }
        
        // set skin flag
        meshFlags |= ToolkitUtil::HasSkin;
    }
}

} // namespace ToolkitUtil