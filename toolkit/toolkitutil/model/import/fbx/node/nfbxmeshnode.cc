//------------------------------------------------------------------------------
//  fbxmeshnode.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "nfbxnode.h"
#include "nfbxmeshnode.h"
#include "model/meshutil/meshbuildervertex.h"
#include "nfbxscene.h"
#include "ufbx/ufbx.h"

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
    , ufbx_node* fbxNode
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
    , const Util::Dictionary<ufbx_node*, SceneNode*>& nodeLookup
    , const ToolkitUtil::ExportFlags flags
)
{
    node->mesh.meshIndex = meshes.Size();

    // Add a single mesh primitive for FBX nodes
    MeshBuilder& mesh = meshes.Emplace();
    
    if (node->fbx.node->materials.count)
    {
        ufbx_material* mat = node->fbx.node->materials[0];
        node->mesh.material = Util::String(mat->name.data, mat->name.length);
    }

    ufbx_mesh* fbxMesh = node->fbx.node->mesh;

    // create mask
    uint meshMask = ToolkitUtil::NoMeshFlags;
    if (fbxMesh->skin_deformers.count > 0)
    {
        meshMask |= ToolkitUtil::HasSkin;
    }

    // tag as physics if parent name is physics, note that this will only work of the direct parent is called physics!
    SceneNode* parent = node->base.parent;
    if (parent != nullptr && parent->base.isPhysics)
    {
        node->mesh.material = "physics";
    }

    if (fbxMesh->uv_sets.count > 1)
        meshMask |= HasMultipleUVs;

    if (fbxMesh->vertex_color.values.count > 0)
        meshMask |= HasVertexColors;

    if (fbxMesh->vertex_tangent.values.count > 0)
        meshMask |= HasTangents;

    if (fbxMesh->vertex_normal.values.count > 0)
        meshMask |= HasNormals;

    // Provoke generation of LOD groups
    ufbx_lod_group* lodGroup = nullptr;
    if (node->fbx.node->parent != nullptr)
    {
        lodGroup = ufbx_as_lod_group(&node->fbx.node->parent->element);
    }

    /* TODO: Generate tangents?
    if (AllBits(meshMask, HasNormals))
    {
        bool res = fbxMesh->GenerateTangentsData(0, true);
        n_assert(res);
        meshMask |= HasTangents;
    }

    */


    // set mask
    node->mesh.meshFlags = (ToolkitUtil::MeshFlags)meshMask;

    // Proceed to extract mesh
    size_t vertexCount = fbxMesh->vertex_position.values.count;
    size_t polyCount = fbxMesh->faces.count;
    size_t uvCount = fbxMesh->uv_sets.count;
    size_t normalCount = fbxMesh->vertex_normal.indices.count;
    size_t tangentCount = fbxMesh->vertex_tangent.indices.count;
    size_t colorCount = fbxMesh->vertex_color.indices.count;

    // this is here just to inform if an artist has forgot to apply a UV set or the mesh has no normals prior to importing
    if (vertexCount > 0)
    {
        n_assert_fmt(uvCount > 0, "You need at least one UV-channel or no shader will be applicable!");
        n_assert_fmt(normalCount > 0, "You need at least one set of normals or no shader will be applicable!");
    }

    // get scale
    uint groupId = MeshCounter++;
    node->mesh.groupId = groupId;

    MeshBuilderVertex::ComponentMask componentMask = 0x0;
    Util::FixedArray<Math::vec4> controlPoints;
    controlPoints.Resize((SizeT)vertexCount);

    mesh.NewMesh(controlPoints.Size(), (SizeT)fbxMesh->num_triangles);
    mesh.SetPrimitiveTopology(CoreGraphics::PrimitiveTopology::TriangleList);
    for (IndexT i = 0; i < controlPoints.Size(); i++)
    {
        ufbx_vec3 v = fbxMesh->vertex_position.values[i];
        controlPoints[i] = FbxToMath(v) * vec4(AdjustedScale, AdjustedScale, AdjustedScale, 1);
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

    ufbx_vertex_vec2* uv0Elements = nullptr, *uv1Elements = nullptr;
    ufbx_vertex_vec4* colorElements = nullptr;
    ufbx_vertex_vec3* normalElements = nullptr, *tangentElements = nullptr;

    // Convert all meshes to map their indexes to once value per vertex
    if (uvCount > 0)
        uv0Elements = &fbxMesh->uv_sets[0].vertex_uv;
    if (uvCount > 1)
        uv1Elements = &fbxMesh->uv_sets[1].vertex_uv;
    if (colorCount > 0)
        colorElements = &fbxMesh->vertex_color;
    if (normalCount > 0)
        normalElements = &fbxMesh->vertex_normal;
    if (tangentCount > 0)
        tangentElements = &fbxMesh->vertex_tangent;

    // setup triangles
    int vertexId = 0; 
    for (int polygonIndex = 0; polygonIndex < polyCount; polygonIndex++)
    {
        const ufbx_face& face = fbxMesh->faces[polygonIndex];
        size_t requiredIndices = (face.num_indices - 2) * 3;
        uint32_t* indices = (uint32_t*)Memory::StackAlloc(requiredIndices * sizeof(uint32));
        uint status = ufbx_triangulate_face(indices, requiredIndices, fbxMesh, face);
        n_assert_fmt(status != 0, "Triangulation failed on polygon %d", polygonIndex);
        for (int tri = 0; tri < requiredIndices / 3; tri++)
        {
            MeshBuilderTriangle meshTriangle;
            for (int triangleVertexIndex = 0; triangleVertexIndex < 3; triangleVertexIndex++)
            {
                // we want to offset the vertex index with the current size of the mesh
                int baseIndex = indices[tri * 3 + triangleVertexIndex];
                int polygonVertex = fbxMesh->vertex_position.indices[baseIndex];
                MeshBuilderVertex meshVertex;
                meshVertex.SetPosition(controlPoints[polygonVertex]);

                // extract uvs from the 0th channel
                Math::vec2 uv = Extract(uv0Elements, baseIndex);
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
                        Math::vec2 uv2 = Extract(uv1Elements, baseIndex);
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
                        Math::vec4 color = Extract(colorElements, baseIndex);
                        meshVertex.SetColor(color);
                    }
                    else
                    {
                        // set vertex color to be white if no vertex colors can be extracted from FBX
                        meshVertex.SetColor(Math::vec4(1));
                    }
                    componentMask |= MeshBuilderVertex::Color;
                }

                Math::vec3 normal = Extract(normalElements, baseIndex);
                meshVertex.SetNormal(normal);
                componentMask |= MeshBuilderVertex::Normals;

                if (AllBits(node->mesh.meshFlags, HasTangents))
                {
                    Math::vec3 tangent = Extract(tangentElements, baseIndex);
                    meshVertex.SetTangent(tangent);
                    //meshVertex.SetSign(-tangent.w);
                    componentMask |= MeshBuilderVertex::Tangents;
                }
                else
                {
                    meshVertex.SetTangent({ 0, 0, 0 });
                    meshVertex.SetSign(0);
                }

                mesh.AddVertex(meshVertex);
                meshTriangle.SetVertexIndex(triangleVertexIndex, vertexId);

                vertexId++;
            }
            mesh.AddTriangle(meshTriangle);
        }
    }

    if (AllBits(node->mesh.meshFlags, HasNormals))
    {
        mesh.CalculateTangents();
        componentMask |= MeshBuilderVertex::Tangents;
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
        if (lodGroup->lod_levels.count > node->mesh.lodIndex - 1)
            node->mesh.minLodDistance = lodGroup->lod_levels[node->mesh.lodIndex - 1].distance;
        else
            node->mesh.minLodDistance = 0;
        if (lodGroup->lod_levels.count > node->mesh.lodIndex)
            node->mesh.maxLodDistance = lodGroup->lod_levels[node->mesh.lodIndex].distance;
        else
            node->mesh.maxLodDistance = 0;
    }
}

//------------------------------------------------------------------------------
/**
    Hmm, maybe a way to check if we have more than 255 indices, then we should switch to using Joint indices as complete uints?
*/
void 
NFbxMeshNode::ExtractSkin(SceneNode* node, Util::FixedArray<Math::uint4>& indices, Util::FixedArray<Math::vec4>& weights, const Util::Dictionary<ufbx_node*, SceneNode*>& nodeLookup, ufbx_mesh* fbxMesh)
{
    size_t vertexCount = fbxMesh->num_vertices;
    size_t skinCount = fbxMesh->skin_deformers.count;
    
    Util::FixedArray<Util::Array<std::tuple<int, float>>> keyWeightPairs((SizeT)vertexCount);
    ufbx_matrix geometricTransform = ufbx_transform_to_matrix(&node->fbx.node->geometry_transform);
    for (int skinIndex = 0; skinIndex < skinCount; skinIndex++)
    {
        ufbx_skin_deformer* skin = fbxMesh->skin_deformers[skinIndex];
        size_t clusterCount = skin->clusters.count;
        node->base.isSkin = true;

        for (int clusterIndex = 0; clusterIndex < clusterCount; clusterIndex++)
        {
            ufbx_skin_cluster* cluster = skin->clusters[clusterIndex];
            ufbx_node* joint = cluster->bone_node;
            SceneNode* jointNode = nodeLookup[joint];
            n_assert(jointNode != nullptr);
            n_assert(jointNode->skeleton.bindMatrix == Math::mat4());
            n_assert(jointNode->type == SceneNode::NodeType::Joint);

            
            ufbx_matrix inversedPose = ufbx_matrix_invert(&cluster->bind_to_world);
            ufbx_matrix transformMatrix = cluster->geometry_to_bone;
            transformMatrix = ufbx_matrix_mul(&inversedPose, &transformMatrix);
            inversedPose = ufbx_matrix_mul(&transformMatrix, &geometricTransform);
            // Calculate inverse transform for skinned vertices
            //ufbx_matrix inversedPose = ufbx_matrix_invert(&cluster->mesh_node_to_bone);
            //ufbx_matrix transformMatrix = cluster->geometry_to_bone;
            //ufbx_matrix geometryMatrix = ufbx_transform_to_matrix(&geometricTransform);
            //jointNode->skeleton.bindMatrix = FbxToMath(ufbx_matrix_mul(&inversedPose, &ufbx_matrix_mul(&transformMatrix, &geometryMatrix)));
            jointNode->skeleton.bindMatrix = FbxToMath(cluster->geometry_to_bone);
            jointNode->skeleton.bindMatrix.position *= Math::vec4(AdjustedScale, AdjustedScale, AdjustedScale, 1);

            size_t clusterVertexIndexCount = cluster->vertices.count;
            for (int vertexIndex = 0; vertexIndex < clusterVertexIndexCount; vertexIndex++)
            {
                int vertex = cluster->vertices[vertexIndex]; 
                float weight = cluster->weights[vertexIndex];
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
