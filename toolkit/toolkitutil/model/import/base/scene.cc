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
    Each vertex component emits a new mesh builder to be exported in the NVX.
    Then, within each vertex component, emit a single node and primitive group per material.

    This will effectively merge together nodes of the same vertex component and material into a single draw.

    Caller takes ownership of the output meshes and has to delete them
*/
void
Scene::OptimizeGraphics(Util::Array<SceneNode*>& outMeshNodes, Util::Array<SceneNode*>& outCharacterNodes, Util::Array<MeshBuilderGroup>& outGroups, Util::Array<MeshBuilder*>& outMeshes)
{
    // Bucket meshes based on components
    Util::Dictionary<MeshBuilderVertex::ComponentMask, Util::Array<SceneNode*>> nodesByComponents;
    for (SceneNode& node : this->nodes)
    {
        if (node.type == SceneNode::NodeType::Mesh)
        {
            if (node.mesh.material != "physics")
                nodesByComponents.Emplace(this->meshes[node.mesh.meshIndex].GetComponents()).Append(&node);
        }
        else if (node.type == SceneNode::NodeType::Joint && node.base.parent == nullptr)
        {
            outCharacterNodes.Append(&node);
        }
    }

    IndexT groupId = 0;

    // Create a mesh builder per each component and emit primitive groups per unique material
    for (IndexT i = 0; i < nodesByComponents.Size(); i++)
    {
        MeshBuilder* builder = n_new(MeshBuilder);
        const Util::Array<SceneNode*>& nodes = nodesByComponents.ValueAtIndex(i);
        builder->SetComponents(nodesByComponents.KeyAtIndex(i));
        builder->SetPrimitiveTopology(this->meshes[nodes[0]->mesh.meshIndex].GetPrimitiveTopology());
        
        // Sort nodes on material, this allows us to merge all meshes in sequence
        std::sort(nodes.begin(), nodes.end(), [](SceneNode* lhs, SceneNode* rhs)
        {
            return lhs->mesh.material > rhs->mesh.material;
        });

        IndexT triangleOffset = 0, numTriangles = 0;
        SceneNode* firstNodeInRange = nullptr;
        MeshBuilderGroup* group;
        for (IndexT j = 0; j < nodes.Size(); j++)
        {
            // Whenever we hit another material, we need a new primitive group
            if (firstNodeInRange == nullptr
                || nodes[j]->mesh.material != firstNodeInRange->mesh.material)
            {
                // Repoint first node in range
                firstNodeInRange = nodes[j];

                // Set group ID within this mesh and the index into the whole mesh resource
                firstNodeInRange->mesh.groupId = groupId++;
                firstNodeInRange->mesh.meshIndex = outMeshes.Size();

                // This is also the output node
                outMeshNodes.Append(firstNodeInRange);

                // Get a new group
                group = &outGroups.Emplace();

                // Add group and set first index
                group->SetFirstTriangleIndex(triangleOffset);
                triangleOffset += numTriangles;
                numTriangles = 0;
            }
            
            // Merge the actual meshes and make sure the group contains the triangles merged
            builder->Merge(this->meshes[nodes[j]->mesh.meshIndex]);
            numTriangles += builder->GetNumTriangles();
            group->SetNumTriangles(numTriangles);
        }
        outMeshes.Append(builder);
    }

    // Go through and split primitive groups if they are skins.
    // This is because each joint index in the ubyte4 compressed data can only
    // access 255 individual joints
    for (IndexT i = 0; i < outMeshNodes.Size(); i++)
    {
        SceneNode* node = outMeshNodes[i];
        if (node->base.isSkin)
        {
            MeshBuilder* mesh = outMeshes[node->mesh.meshIndex];
            Util::Set<IndexT> joints;
            SizeT baseTriangle = 0;
            SizeT numTriangles = 0;
            MeshBuilderGroup group;
            group.SetFirstTriangleIndex(baseTriangle);
            for (IndexT j = 0; j < mesh->GetNumTriangles(); j++)
            {
                const MeshBuilderTriangle& tri = mesh->TriangleAt(j);
                const MeshBuilderVertex& vtx0 = mesh->VertexAt(tri.GetVertexIndex(0));
                const MeshBuilderVertex& vtx1 = mesh->VertexAt(tri.GetVertexIndex(1));
                const MeshBuilderVertex& vtx2 = mesh->VertexAt(tri.GetVertexIndex(2));
                
                // Create set of unique joint
                Util::Set<IndexT> uniqueJoints;
                for (IndexT k = 0; k < 4; k++)
                {
                    if (joints.FindIndex(vtx0.attributes.skin.indices.v[k]) == InvalidIndex)
                        uniqueJoints.Add(vtx0.attributes.skin.indices.v[k]);
                    if (joints.FindIndex(vtx1.attributes.skin.indices.v[k]) == InvalidIndex)
                        uniqueJoints.Add(vtx1.attributes.skin.indices.v[k]);
                    if (joints.FindIndex(vtx2.attributes.skin.indices.v[k]) == InvalidIndex)
                        uniqueJoints.Add(vtx2.attributes.skin.indices.v[k]);
                }
                numTriangles++;

                // If we passed 256 joints, create a new fragment and reset the joints list
                if (joints.Size() + uniqueJoints.Size() >= 256)
                {
                    // Create new group and joint lookup whenever we hit the max allowed
                    node->skin.skinFragments.Append(outGroups.Size());
                    node->skin.jointLookup.Append(joints);

                    group.SetFirstTriangleIndex(baseTriangle);
                    group.SetNumTriangles(numTriangles);
                    outGroups.Append(group);

                    // The joints pushing us over the boundary is saved for the next iteration
                    baseTriangle = numTriangles;
                    numTriangles = 0;
                    joints = uniqueJoints;
                }
                else
                {
                    // If we are still within 256, simply add joints
                    for (IndexT k = 0; k < uniqueJoints.Size(); k++)
                        joints.Add(uniqueJoints.KeyAtIndex(k));
                }                
            }

            // Add the last group
            node->skin.skinFragments.Append(outGroups.Size());
            node->skin.jointLookup.Append(joints);
            group.SetFirstTriangleIndex(baseTriangle);
            group.SetNumTriangles(numTriangles);
            outGroups.Append(group);

            // Fixup joint indices, by moving the range of joints per fragment to 0-255
            int jointOffset = 0;
            for (IndexT i = 0; i < node->skin.skinFragments.Size(); i++)
            {
                // The first joint is going to be the lowest value in our range
                const Util::Set<IndexT>& lookup = node->skin.jointLookup[i];
                const MeshBuilderGroup& group = outGroups[node->skin.skinFragments[i]];
                for (IndexT i = 0; i < group.GetNumTriangles(); i++)
                {
                    const MeshBuilderTriangle& tri = mesh->TriangleAt(group.GetFirstTriangleIndex() + i);
                    MeshBuilderVertex& vtx0 = mesh->VertexAt(tri.GetVertexIndex(0));
                    MeshBuilderVertex& vtx1 = mesh->VertexAt(tri.GetVertexIndex(1));
                    MeshBuilderVertex& vtx2 = mesh->VertexAt(tri.GetVertexIndex(2));

                    // Using the joint lookup table, remap the vertex joint indices
                    for (IndexT k = 0; k < 4; k++)
                    {
                        IndexT idx0, idx1, idx2;
                        idx0 = lookup.FindIndex(vtx0.attributes.skin.indices.v[k]);
                        n_assert(idx0 != InvalidIndex);
                        vtx0.attributes.skin.remapIndices.v[k] = idx0;

                        idx1 = lookup.FindIndex(vtx1.attributes.skin.indices.v[k]);
                        n_assert(idx1 != InvalidIndex);
                        vtx1.attributes.skin.remapIndices.v[k] = idx1;

                        idx2 = lookup.FindIndex(vtx2.attributes.skin.indices.v[k]);
                        n_assert(idx2 != InvalidIndex);
                        vtx2.attributes.skin.remapIndices.v[k] = idx2;
                    }
                }
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
Scene::OptimizePhysics(Util::Array<SceneNode*>& outNodes, MeshBuilder*& outMesh)
{
    MeshBuilder* mesh = n_new(MeshBuilder);
    for (SceneNode& node : this->nodes)
    {
        if (node.mesh.material == "physics")
        {
            mesh->Merge(this->meshes[node.mesh.meshIndex]);
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

    std::function<void(Util::Array<AnimBuilderCurve>&, SceneNode*)> collectCurves = [&](Util::Array<AnimBuilderCurve>& curves, SceneNode* node)
    {
        curves.Append(node->anim.translationCurve);
        curves.Append(node->anim.rotationCurve);
        curves.Append(node->anim.scaleCurve);

        for (auto child : node->base.children)
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

} // namespace ToolkitUtil
