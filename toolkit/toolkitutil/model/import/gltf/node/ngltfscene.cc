//------------------------------------------------------------------------------
//  ngltfscene.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "ngltfscene.h"
#include "model/modelutil/modeldatabase.h"
#include "model/skeletonutil/skeletonbuilder.h"
#include "model/n3util/n3modeldata.h"
#include "timing/timer.h"
#include "model/import/base/uniquestring.h"
#include <array>
#include <functional>

using namespace Util;
using namespace ToolkitUtil;
namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
NglTFScene::NglTFScene()
{
}

//------------------------------------------------------------------------------
/**
*/
NglTFScene::~NglTFScene()
{
}

//------------------------------------------------------------------------------
/**
*/
void
NglTFScene::Setup(Gltf::Document* scene
                  , const ToolkitUtil::ExportFlags& exportFlags
                  , float scale)
{
    n_assert(scene);

    // set export settings
    this->flags = flags;

    // set scale
    SceneScale = scale;
    AdjustedScale = 1.0f / scale;

    Util::Dictionary<int, IndexT> jointNodeToIndex;

    Timing::Timer timer;

    // Extract all skeletons to the scene list
    if (!scene->skins.IsEmpty())
    {
        timer.Start();
        for (auto const& skin : scene->skins)
        {
            // Build skeleton
            SkeletonBuilder& skel = this->skeletons.Emplace();

            // First, create all joint that we know we'll need.
            skel.joints.Resize(skin.joints.Size());

            IndexT index = 0;
            for (auto const jointNode : skin.joints)
            {
                auto const& node = scene->nodes[jointNode];
                
                // Create joint map
                jointNodeToIndex.Add(jointNode, index);

                Math::mat4 bind;

                if (node.hasTRS)
                {
                    bind = Math::affine(node.scale, Math::vec3(0), node.rotation, node.translation);
                }
                else
                {
                    bind = node.matrix;
                }

                ToolkitUtil::Joint& joint = skel.joints[index];
                joint.index = index;
                joint.name = node.name;
                joint.bind = bind;
                
                index++;
            }

            // update parents for each joint
            for (auto const jointNode : skin.joints)
            {
                auto const& node = scene->nodes[jointNode];
                IndexT parent = jointNodeToIndex[jointNode];
                for (auto const child : node.children)
                {
                    IndexT childIndex = jointNodeToIndex[child];
                    skel.joints[childIndex].parent = parent;
                }
            }
        }
        timer.Stop();
        n_printf("    [glTF - Skeletons read (%d, %d ms)]\n", scene->skins.Size(), timer.GetTime() * 1000);
    }

    // We need to load our animations and add them to the scene animation list
    if (!scene->animations.IsEmpty())
    {
        timer.Reset();
        timer.Start();
        AnimBuilder animBuilder;
        for (auto const& animation : scene->animations)
        {
            AnimBuilderClip clip;
            clip.SetName(animation.name);
            
            for (auto const& channel : animation.channels)
            {
                AnimBuilderCurve curve;
                Gltf::Animation::Sampler const& sampler = animation.samplers[channel.sampler];
                
                int jointNode = channel.target.node;
                
                //this->scene->accessors[sampler.output].count
            }
        }
        timer.Stop();
        n_printf("    [glTF - Animations read (%d, %d ms)]\n", scene->animations.Size(), timer.GetTime() * 1000);
    }

    // Traverse node hierarchies and count how many SceneNodes we will need
    const int numRootNodes = scene->scenes[scene->scene].nodes.Size();
    int numNodes = numRootNodes;
    int nodeCounter = 0;
    int meshCounter = 0;
    for (int i = 0; i < numRootNodes; i++)
    {
        Gltf::Node* node = &scene->nodes[scene->scenes[scene->scene].nodes[i]];

        std::function<void(Gltf::Node*, int& count)> counter = [&](Gltf::Node* node, int& count)
        {
            if (node->mesh != -1)
            {
                // glTF meshes can contain several primitives, which has to be a unique SceneNode each
                // due to the fact they may vary in vertex layouts
                Gltf::Mesh* mesh = &scene->meshes[node->mesh];
                meshCounter += mesh->primitives.Size();
                count += mesh->primitives.Size() + 1;
            }
            else
                count++;

            for (int i = 0; i < node->children.Size(); i++)
            {
                counter(&scene->nodes[node->children[i]], count);
            }
        };

        counter(node, nodeCounter);
    }
    this->nodes.Reserve(nodeCounter);
    this->meshes.Resize(meshCounter);

    // Create a mapping between meshes and primitives
    Util::Dictionary<IndexT, Util::Array<SceneNode*>> meshToNodeMapping;

    IndexT basePrimitiveIndex = 0;
    std::function<void(Gltf::Node*, SceneNode*)> nodeSetup = [&](Gltf::Node* gltfNode, SceneNode* parent)
    {
        // If we encounter a mesh, we need to emit one SceneNode per mesh primitive
        SceneNode* node = nullptr;
        if (gltfNode->mesh != -1)
        {
            // Create one transform node to only extract transforms
            node = &this->nodes.Emplace();
            node->Setup(SceneNode::NodeType::Transform);
            NglTFNode::Setup(gltfNode, node, parent);
            node->base.name = gltfNode->name;
            if (node->base.name.IsEmpty())
            {
                node->base.name = UniqueString::New("unnamed");
            }

            Util::Array<SceneNode*> primitiveNodes;
            const Gltf::Mesh& mesh = scene->meshes[gltfNode->mesh];
            primitiveNodes.Reserve(mesh.primitives.Size());

            // Create one mesh node per primitive node, and parent them to the transform node above
            for (IndexT i = 0; i < mesh.primitives.Size(); i++)
            {
                SceneNode* primitiveNode = &this->nodes.Emplace();
                primitiveNode->Setup(SceneNode::NodeType::Mesh);
                NglTFNode::Setup(gltfNode, primitiveNode, node);
                primitiveNode->base.name = Util::String::Sprintf("%s:%d", node->base.name.AsCharPtr(), i);

                // Kill transforms on primitive as it's owned by the parent transform
                primitiveNode->base.rotation = Math::quat();
                primitiveNode->base.scale = Math::vec3(1);
                primitiveNode->base.translation = Math::vec3(0);
                primitiveNodes.Append(primitiveNode);

                if (gltfNode->skin != -1)
                    primitiveNode->skeleton.skeletonIndex = gltfNode->skin;
            }
            NglTFMesh::Setup(this->meshes, &scene->meshes[gltfNode->mesh], scene, flags, basePrimitiveIndex, gltfNode->mesh, primitiveNodes.Begin());
            basePrimitiveIndex += mesh.primitives.Size();
        }
        else
        {
            // If just a transform, the glTF node maps to SceneNodes 1:1
            node = &this->nodes.Emplace();
            node->Setup(SceneNode::NodeType::Transform);
            NglTFNode::Setup(gltfNode, node, parent);
            node->base.name = gltfNode->name;
        }

        // TODO: Add support for the rest of the nodes we might want, like LOD


        // Recurse down to children
        for (int i = 0; i < gltfNode->children.Size(); i++)
        {
            nodeSetup(&scene->nodes[gltfNode->children[i]], node);
        }
    };

    // Iterate over nodes again and setup exporter nodes
    for (int i = 0; i < numRootNodes; i++)
    {
        Gltf::Node* gltfNode = &scene->nodes[scene->scenes[scene->scene].nodes[i]];
        nodeSetup(gltfNode, nullptr);
    }
    n_printf("    [glTF - Parsing done (%d)]\n", scene->meshes.Size());
}

} // namespace ToolkitUtil
