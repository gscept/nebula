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
                    node->skin.jointLookup.Append(joints.KeysAsArray());

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
            node->skin.jointLookup.Append(joints.KeysAsArray());
            group.SetFirstTriangleIndex(baseTriangle);
            group.SetNumTriangles(numTriangles);
            outGroups.Append(group);
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
bool 
Scene::SplitAnimationCurves(SceneNode* node, const Util::Array<AnimBuilderCurve>& curves, ToolkitUtil::AnimBuilder& anim, const Ptr<ToolkitUtil::ModelAttributes>& attributes)
{
    // split if we have attributes for this stack and resource
    if (attributes.isvalid() && attributes->HasTake(node->anim.take))
    {
        const Ptr<Take>& take = attributes->GetTake(node->anim.take);

        // we might have an empty take, in this case, just return false
        if (take->GetClips().Size() == 0)
        {
            return false;
        }

        const Util::Array<Ptr<Clip>>& clips = take->GetClips();
        for (int splitIndex = 0; splitIndex < clips.Size(); splitIndex++)
        {
            const Ptr<Clip>& split = clips[splitIndex];
            AnimBuilderClip clip;
            clip.SetName(split->GetName());
            clip.SetKeyDuration(KeysPerMS);
            int clipKeyCount = split->GetEnd() - split->GetStart();
            if (clipKeyCount == 0) continue; // ignore empty clips
            clip.SetNumKeys(clipKeyCount);
            clip.SetPreInfinityType(split->GetPreInfinity() == Clip::Constant ? CoreAnimation::InfinityType::Constant : CoreAnimation::InfinityType::Cycle);
            clip.SetPostInfinityType(split->GetPostInfinity() == Clip::Constant ? CoreAnimation::InfinityType::Constant : CoreAnimation::InfinityType::Cycle);
            clip.SetStartKeyIndex(0);

            for (int curveIndex = 0; curveIndex < curves.Size(); curveIndex += 3)
            {
                const AnimBuilderCurve& curveX = curves[curveIndex];
                const AnimBuilderCurve& curveY = curves[curveIndex + 1];
                const AnimBuilderCurve& curveZ = curves[curveIndex + 2];

                AnimBuilderCurve splitX;
                AnimBuilderCurve splitY;
                AnimBuilderCurve splitZ;

                splitX.SetCurveType(curveX.GetCurveType());
                splitY.SetCurveType(curveY.GetCurveType());
                splitZ.SetCurveType(curveZ.GetCurveType());

                if (curveX.IsStatic())
                {
                    splitX.SetStaticKey(curveX.GetStaticKey());
                    splitX.SetStatic(true);
                    splitX.SetActive(true);
                }
                else
                {
                    splitX.ResizeKeyArray(clipKeyCount);
                    splitX.SetActive(true);
                    splitX.SetStatic(false);
                }
                if (curveY.IsStatic())
                {
                    splitY.SetStaticKey(curveY.GetStaticKey());
                    splitY.SetStatic(true);
                    splitX.SetActive(true);
                }
                else
                {
                    splitY.ResizeKeyArray(clipKeyCount);
                    splitY.SetActive(true);
                    splitY.SetStatic(false);
                }
                if (curveZ.IsStatic())
                {
                    splitZ.SetStaticKey(curveZ.GetStaticKey());
                    splitZ.SetStatic(true);
                    splitX.SetActive(true);
                }
                else
                {
                    splitZ.ResizeKeyArray(clipKeyCount);
                    splitZ.SetActive(true);
                    splitZ.SetStatic(false);
                }

                int splitIndex = 0;
                for (int keyIndex = split->GetStart(); (keyIndex < split->GetEnd()) && (keyIndex < node->anim.span); keyIndex++)
                {
                    if (!splitX.IsStatic())
                    {
                        splitX.SetKey(splitIndex, curveX.GetKey(keyIndex));
                    }
                    if (!splitY.IsStatic())
                    {
                        splitY.SetKey(splitIndex, curveY.GetKey(keyIndex));
                    }
                    if (!splitZ.IsStatic())
                    {
                        splitZ.SetKey(splitIndex, curveZ.GetKey(keyIndex));
                    }
                    splitIndex++;
                }

                clip.AddCurve(splitX);
                clip.AddCurve(splitY);
                clip.AddCurve(splitZ);
            }

            const Util::Array<Ptr<ClipEvent>>& events = split->GetEvents();
            for (int eventIndex = 0; eventIndex < events.Size(); eventIndex++)
            {
                const Ptr<ClipEvent>& ev = events[eventIndex];

                CoreAnimation::AnimEvent animEvent;
                animEvent.SetCategory(ev->GetCategory());
                animEvent.SetName(ev->GetName());

                // solve the marker, in one case we have the exact number of ticks, in the other we have frames
                int marker = ev->GetMarker();
                ClipEvent::MarkerType type = ev->GetMarkerType();
                switch (type)
                {
                    case ClipEvent::Ticks:
                        animEvent.SetTime(Math::max(marker - 1, 0) / clip.GetKeyDuration());
                        break;
                    case ClipEvent::Frames:
                        animEvent.SetTime(Math::max(marker - 1, 0));
                        break;
                }
                clip.AddEvent(animEvent);
            }

            anim.AddClip(clip);
        }
        return true;
    }
    return false;
}

} // namespace ToolkitUtil
