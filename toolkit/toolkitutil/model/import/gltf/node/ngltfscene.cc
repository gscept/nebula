//------------------------------------------------------------------------------
//  ngltfscene.cc
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "ngltfscene.h"
#include "model/modelutil/modeldatabase.h"
#include "model/skeletonutil/skeletonbuilder.h"
#include "model/n3util/n3modeldata.h"
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

    // Extract all skeletons to the scene list
    if (!scene->skins.IsEmpty())
    {
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
    }

    // We need to load our animations and add them to the scene animation list
    if (!scene->animations.IsEmpty())
    {
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
    }

    // Create a mapping between meshes and primitives
    Util::Dictionary<IndexT, Util::Array<IndexT>> meshToPrimitiveMapping;

    // Extract meshes
    if (!scene->meshes.IsEmpty())
    {
        IndexT meshIndex = 0;
        IndexT primitiveIndex = 0;
        for (auto const& mesh : scene->meshes)
        {
            Util::Array<IndexT> primitives;
            for (IndexT i = 0; i < mesh.primitives.Size(); i++)
            {
                primitives.Append(primitiveIndex++);
            }
            meshToPrimitiveMapping.Add(meshIndex++, primitives);
            NglTFMesh::Setup(this->meshes, &mesh, scene, flags);
        }
    }
    
    // Traverse node hierarchies and count how many SceneNodes we will need
    const int numRootNodes = scene->scenes[scene->scene].nodes.Size();
    int numNodes = numRootNodes;
    for (int i = 0; i < numRootNodes; i++)
    {
        Gltf::Node* node = &scene->nodes[scene->scenes[scene->scene].nodes[i]];

        std::function<void(Gltf::Node*, int& count)> counter = [&](Gltf::Node* node, int& count)
        {
            if (node->mesh)
            {
                // glTF meshes can contain several primitives, which has to be a unique SceneNode each
                // due to the fact they may vary in vertex layouts
                Gltf::Mesh* mesh = &scene->meshes[node->mesh];
                count += mesh->primitives.Size();
            }
            else
                count++;

            for (int i = 0; i < node->children.Size(); i++)
            {
                counter(&scene->nodes[node->children[i]], count);
            }
        };

        counter(node, numNodes);
    }
    this->nodes.Reserve(numNodes);

    std::function<void(Gltf::Node*)> nodeSetup = [&](Gltf::Node* gltfNode)
    {
        // If we encounter a mesh, we need to emit one SceneNode per mesh primitive
        if (gltfNode->mesh != -1)
        {
            Util::Array<IndexT> primitives = meshToPrimitiveMapping[gltfNode->mesh];
            for (IndexT j = 0; j < primitives.Size(); j++)
            {
                SceneNode& node = this->nodes.Emplace();
                node.Setup(SceneNode::NodeType::Mesh);
                NglTFNode::Setup(gltfNode, &node);

                node.mesh.meshIndex = primitives[j];

                if (gltfNode->skin != -1)
                    node.skeleton.skeletonIndex = gltfNode->skin;
            }
        }
        else
        {
            // If just a transform, the glTF node maps to SceneNodes 1:1
            SceneNode& node = this->nodes.Emplace();
            node.Setup(SceneNode::NodeType::Transform);
            NglTFNode::Setup(gltfNode, &node);
        }

        // TODO: Add support for the rest of the nodes we might want, like LOD


        // Recurse down to children
        for (int i = 0; i < gltfNode->children.Size(); i++)
        {
            nodeSetup(&scene->nodes[gltfNode->children[i]]);
        }
    };

    // Iterate over nodes again and setup exporter nodes
    for (int i = 0; i < numRootNodes; i++)
    {
        Gltf::Node* gltfNode = &scene->nodes[scene->scenes[scene->scene].nodes[i]];
        nodeSetup(gltfNode);
    }
}

} // namespace ToolkitUtil
