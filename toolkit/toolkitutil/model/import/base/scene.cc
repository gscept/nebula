//------------------------------------------------------------------------------
//  @file scene.cc
//  @copyright (C) 2022 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "scene.h"
#include "model/meshutil/meshbuilder.h"
#include "model/skeletonutil/skeletonbuilder.h"
#include "model/modelutil/clip.h"
#include "model/modelutil/clipevent.h"
#include "model/modelutil/take.h"
#include "model/modelutil/modelattributes.h"
#include "util/set.h"

#include <algorithm>

namespace ToolkitUtil
{

float SceneScale = 1.0f;
float AdjustedScale = 1.0f;
float AnimationFrameRate = 24.0f;
int KeysPerMS = 40;

//------------------------------------------------------------------------------
/**
*/
Scene::Scene()
{
    // Do nothing
}

//------------------------------------------------------------------------------
/**
*/
Scene::~Scene()
{
    // Do nothing
}

//------------------------------------------------------------------------------
/**
    Merge nodes based on two criteria. First criteria is vertex components. 
    Each vertex component emits a new mesh builder to be exported in the namsh.
    Then, within each vertex component, emit a single node and primitive group per material.

    This will effectively merge together nodes of the same vertex component and material into a single draw.

    Caller takes ownership of the output meshes and has to delete them
*/
void
Scene::OptimizeGraphics(
    ToolkitUtil::Logger* logger
    , Util::Array<SceneNode*>& outMeshNodes
    , Util::Array<MeshBuilder*>& outMeshes
)
{
    // Bucket meshes based on components
    Util::Dictionary<MeshBuilderVertex::ComponentMask, Util::Array<SceneNode*>> nodesByComponents;
    for (SceneNode& node : this->nodes)
    {
        if (node.type == SceneNode::NodeType::Mesh)
        {
            if (node.mesh.material != "physics")
            {
                MeshBuilder& mesh = this->meshes[node.mesh.meshIndex];
                if (MeshBuilderVertex::GetVertexLayoutType(mesh.GetComponents()) == CoreGraphics::VertexLayoutType::Invalid)
                {
                    MeshBuilderVertex::ComponentMask components = MeshBuilderVertex::Components::Position | MeshBuilderVertex::Components::Uvs | MeshBuilderVertex::Components::Normals | MeshBuilderVertex::Components::Tangents;
                    const MeshBuilderVertex::ComponentMask skinComponents = MeshBuilderVertex::Components::SkinWeights | MeshBuilderVertex::Components::SkinIndices;

                    // There is no layout combining skinning with vertex colors or secondary UVs (both are imported by default).
                    // Keep the skin, the node is still written as a skin and the skinned shaders need the weights and joint indices
                    if (AllBits(mesh.GetComponents(), skinComponents))
                    {
                        logger->Warning("Mesh node '%s' is skinned, skinned meshes don't support vertex colors or secondary UVs, dropping them\n", node.base.name.AsCharPtr());
                        components |= skinComponents;
                    }
                    else
                    {
                        logger->Warning("Mesh node '%s' has unknown vertex layout, reverting to Normal\n", node.base.name.AsCharPtr());
                    }
                    mesh.SetComponents(components);
                }
                nodesByComponents.Emplace(this->meshes[node.mesh.meshIndex].GetComponents()).Append(&node);
            }
        }
    }

    
    // Create a mesh builder per each component and emit primitive groups per unique material
    for (IndexT i = 0; i < nodesByComponents.Size(); i++)
    {
        Util::Array<MeshBuilderGroup> groups;
        IndexT groupId = 0;

        MeshBuilder* builder = new MeshBuilder;
        const Util::Array<SceneNode*>& nodes = nodesByComponents.ValueAtIndex(i);
        builder->SetComponents(nodesByComponents.KeyAtIndex(i));
        builder->SetPrimitiveTopology(this->meshes[nodes[0]->mesh.meshIndex].GetPrimitiveTopology());

        // Sort nodes on material, this allows us to merge all meshes in sequence
        std::sort(nodes.begin(), nodes.end(), [](SceneNode* lhs, SceneNode* rhs)
        {
            return lhs->mesh.material < rhs->mesh.material;
        });

        IndexT triangleOffset = 0, numTriangles = 0;
        SceneNode* firstNodeInRange = nullptr;
        MeshBuilderGroup* group;
        for (IndexT j = 0; j < nodes.Size(); j++)
        {
            auto meshToMerge = this->meshes[nodes[j]->mesh.meshIndex];

            // Since we're flattening the scene, integrate the transform in the mesh
            meshToMerge.Transform(nodes[j]->base.globalTransform);     
            nodes[j]->base.rotation = Math::quat();
            nodes[j]->base.scale = Math::vec3(1);
            nodes[j]->base.translation = Math::vec3(0);

            // Whenever we hit another material, we need a new primitive group
            if (firstNodeInRange == nullptr
                || nodes[j]->mesh.material != firstNodeInRange->mesh.material)
            {
                // Repoint first node in range
                firstNodeInRange = nodes[j];

                // Set group ID within this mesh and the index into the whole mesh resource
                firstNodeInRange->mesh.groupId = groupId++;
                firstNodeInRange->mesh.meshIndex = outMeshes.Size();
                firstNodeInRange->base.boundingBox.begin_extend();

                // This is also the output node
                outMeshNodes.Append(firstNodeInRange);

                // Get a new group
                group = &groups.Emplace();

                // Add group and set first index
                triangleOffset += numTriangles;
                group->SetFirstTriangleIndex(triangleOffset);
                numTriangles = 0;
            }

            // Calculate the new triangle count for our current group
            numTriangles += meshToMerge.GetNumTriangles();
            group->SetNumTriangles(numTriangles);

            firstNodeInRange->base.boundingBox.extend(meshToMerge.ComputeBoundingBox());

            // Merge meshes
            builder->Merge(meshToMerge);
        }
        builder->SetPrimitiveGroups(groups);
        outMeshes.Append(builder);
    }

    // Split the primitive group of each skin into fragments of at most 256 joints, the joint indices are stored as ubyte4.
    // The palette of a fragment is uploaded packed, holding only its joints, so the vertices index into it rather than into the skeleton.
    // The vertices of a node aren't shared with other nodes, merging copies them
    for (SceneNode* node : outMeshNodes)
    {
        if (!node->base.isSkin)
            continue;

        MeshBuilder* mesh = outMeshes[node->mesh.meshIndex];
        Util::Array<MeshBuilderGroup> groups = mesh->GetPrimitiveGroups();
        const MeshBuilderGroup nodeGroup = groups[node->mesh.groupId];
        const IndexT firstTriangle = nodeGroup.GetFirstTriangleIndex();
        const IndexT endTriangle = firstTriangle + nodeGroup.GetNumTriangles();

        node->skin.skinFragments.Clear();
        node->skin.jointLookup.Clear();

        // The first fragment keeps the node's group, further ones get groups of their own covering the rest of its triangles
        Util::Set<IndexT> joints;
        IndexT fragmentStart = firstTriangle;
        auto closeFragment = [&](IndexT fragmentEnd)
        {
            IndexT groupIndex = node->mesh.groupId;
            if (!node->skin.skinFragments.IsEmpty())
            {
                groupIndex = groups.Size();
                groups.Append(nodeGroup);
            }
            groups[groupIndex].SetFirstTriangleIndex(fragmentStart);
            groups[groupIndex].SetNumTriangles(fragmentEnd - fragmentStart);
            node->skin.skinFragments.Append(groupIndex);
            node->skin.jointLookup.Append(joints);
        };

        for (IndexT j = firstTriangle; j < endTriangle; j++)
        {
            const MeshBuilderTriangle& tri = mesh->TriangleAt(j);
            Util::Set<IndexT> newJoints;
            for (IndexT v = 0; v < 3; v++)
            {
                const MeshBuilderVertex& vtx = mesh->VertexAt(tri.GetVertexIndex(v));
                for (IndexT k = 0; k < 4; k++)
                {
                    if (!joints.Contains(vtx.attributes.skin.indices.v[k]))
                        newJoints.Add(vtx.attributes.skin.indices.v[k]);
                }
            }

            // The triangle starts a new fragment if its joints don't fit
            if (joints.Size() + newJoints.Size() > 256)
            {
                closeFragment(j);
                fragmentStart = j;
                joints.Clear();
            }
            for (IndexT k = 0; k < newJoints.Size(); k++)
                joints.Add(newJoints.KeyAtIndex(k));
        }
        closeFragment(endTriangle);
        mesh->SetPrimitiveGroups(groups);

        // Remap the joint indices into the palette of the vertex's fragment. A vertex used by triangles of two fragments
        // can only index one palette, so the later fragment gets a copy of it
        Util::Array<IndexT> vertexFragment;
        vertexFragment.Fill(0, mesh->GetNumVertices(), InvalidIndex);
        for (IndexT fragment = 0; fragment < node->skin.skinFragments.Size(); fragment++)
        {
            const Util::Set<IndexT>& lookup = node->skin.jointLookup[fragment];
            const MeshBuilderGroup& group = groups[node->skin.skinFragments[fragment]];
            for (IndexT j = 0; j < group.GetNumTriangles(); j++)
            {
                MeshBuilderTriangle& tri = mesh->TriangleAt(group.GetFirstTriangleIndex() + j);
                for (IndexT v = 0; v < 3; v++)
                {
                    IndexT vertexIndex = tri.GetVertexIndex(v);
                    if (vertexFragment[vertexIndex] == fragment)
                        continue;
                    if (vertexFragment[vertexIndex] != InvalidIndex)
                    {
                        // copied before adding, adding may move the vertices
                        const MeshBuilderVertex copy = mesh->VertexAt(vertexIndex);
                        vertexIndex = mesh->GetNumVertices();
                        mesh->AddVertex(copy);
                        vertexFragment.Append(InvalidIndex);
                        tri.SetVertexIndex(v, vertexIndex);
                    }
                    vertexFragment[vertexIndex] = fragment;

                    MeshBuilderVertex& vtx = mesh->VertexAt(vertexIndex);
                    for (IndexT k = 0; k < 4; k++)
                    {
                        const IndexT index = lookup.FindIndex(vtx.attributes.skin.indices.v[k]);
                        n_assert(index != InvalidIndex);
                        vtx.attributes.skin.remapIndices.v[k] = index;
                    }
                }
            }
        }

        // The lookups above use the skeleton indices, so they're only replaced once all are remapped
        for (IndexT vertexIndex = 0; vertexIndex < vertexFragment.Size(); vertexIndex++)
        {
            if (vertexFragment[vertexIndex] != InvalidIndex)
            {
                MeshBuilderVertex& vtx = mesh->VertexAt(vertexIndex);
                vtx.attributes.skin.indices = vtx.attributes.skin.remapIndices;
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
Scene::OptimizePhysics(Util::Array<SceneNode*>& outNodes, Util::Array<MeshBuilder*>& outMeshes)
{
    for (SceneNode& node : this->nodes)
    {
        if (node.mesh.material == "physics")
        {
            outMeshes.Append(&this->meshes[node.mesh.meshIndex]);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
Scene::GenerateClip(SceneNode* node, AnimBuilder& animBuilder, const Util::String& name)
{
    Util::Array<AnimBuilderCurve> curves;

    std::function<void(Util::Array<AnimBuilderCurve>&, SceneNode*)> collectCurves = [&](Util::Array<AnimBuilderCurve>& curves, SceneNode* curNode)
    {
        curves.Append(curNode->anim.translationCurve);
        curves.Append(curNode->anim.rotationCurve);
        curves.Append(curNode->anim.scaleCurve);

        for (auto child : curNode->base.children)
            collectCurves(curves, child);
    };
    collectCurves(curves, node);

    // If split fails, just add the whole animation
    AnimBuilderClip clip;
    clip.firstCurveOffset = 0;
    clip.numCurves = curves.Size();
    clip.firstEventOffset = 0;
    clip.numEvents = 0;
    clip.firstVelocityCurveOffset = 0;
    clip.numVelocityCurves = 0;

    // Transfer ownership of curves to the builder
    animBuilder.curves = std::move(curves);

    int numCurves = animBuilder.curves.Size();
    clip.duration = 0;

    for (const auto& curve : animBuilder.curves)
        if (curve.numKeys > 0)
            clip.duration = Math::max(clip.duration, animBuilder.keyTimes[curve.firstTimeOffset + curve.numKeys - 1]);

    clip.SetName(name);
    animBuilder.AddClip(clip);
}

//------------------------------------------------------------------------------
/**
*/
void 
Scene::SetupSkeletons()
{
    for (IndexT i = 0; i < this->nodes.Size(); i++)
    {
        if (this->nodes[i].skeleton.isSkeletonRoot)
        {
            IndexT counter = 0;
            std::function<void(SceneNode*, IndexT&)> convertFunc = [&](SceneNode* node, IndexT& counter)
            {
                node->skeleton.jointIndex = counter++;
                if (node->base.parent != nullptr)
                    node->skeleton.parentIndex = node->base.parent->skeleton.jointIndex;

                for (auto& child : node->base.children)
                    convertFunc(child, counter);
            };
            convertFunc(&this->nodes[i], counter);
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Scene::ExtractSkeletons()
{
    for (IndexT i = 0; i < this->nodes.Size(); i++)
    {
        if (this->nodes[i].skeleton.isSkeletonRoot)
        {
            this->nodes[i].skeleton.skeletonIndex = this->skeletons.Size();
            SkeletonBuilder& builder = this->skeletons.Emplace();

            Math::mat4 rootTransform = Math::mat4();
            if (this->nodes[i].base.parent != nullptr)
            {
                // Bake it with parent transform as base
                rootTransform = Math::inverse(this->nodes[i].base.parent->base.globalTransform);
            }

            std::function<void(SceneNode*, SkeletonBuilder&)> convertFunc = [&](SceneNode* node, SkeletonBuilder& builder)
            {
                Joint joint;
                joint.name = node->base.name;
                // bake skeleton root's parent into the bind matrices
                joint.bind = node->skeleton.bindMatrix * rootTransform;
                joint.translation = node->base.translation;
                joint.rotation = node->base.rotation;
                joint.scale = node->base.scale;
                joint.index = node->skeleton.jointIndex;
                joint.parent = node->skeleton.parentIndex;
                builder.joints.Append(joint);

                for (auto& child : node->base.children)
                    convertFunc(child, builder);
            };
            convertFunc(&this->nodes[i], builder);
        }
    }
}

} // namespace ToolkitUtil
