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
#include "math/scalar.h"

using namespace Util;
using namespace ToolkitUtil;
namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
CoreAnimation::CurveType::Code
GltfPathToCurveType(Util::String const& path)
{
    if (path == "translation")
    {
        return CoreAnimation::CurveType::Translation;
    }
    else if (path == "rotation")
    {
        return CoreAnimation::CurveType::Rotation;
    }
    else if (path == "scale")
    {
        return CoreAnimation::CurveType::Scale;
    }
    else if (path == "weights")
    {
        n_error("Morph targets are currently not supported!\n");
    }
}

//------------------------------------------------------------------------------
/**
    returns value, increments bytesRead by the amount of bytes read
*/
float
ReadCurveValue(byte const* const data, Gltf::Accessor::ComponentType componentType, uint64_t& bytesRead)
{
    switch (componentType)
    {
    case Gltf::Accessor::ComponentType::Float:
        bytesRead += sizeof(float);
        return *((float*)data);
    case Gltf::Accessor::ComponentType::Byte:
        bytesRead += sizeof(int8_t);
        return Math::normtofloat(*((int8_t*)data));
    case Gltf::Accessor::ComponentType::UnsignedByte:
        bytesRead += sizeof(uint8_t);
        return Math::normtofloat(*((uint8_t*)data));
    case Gltf::Accessor::ComponentType::Short:
        bytesRead += sizeof(int16_t);
        return Math::normtofloat(*((int16_t*)data));
    case Gltf::Accessor::ComponentType::UnsignedShort:
        bytesRead += sizeof(uint16_t);
        return Math::normtofloat(*((uint16_t*)data));
    case Gltf::Accessor::ComponentType::UnsignedInt: // this isn't supported by gltf animations
    default:
        n_error("ERROR: Invalid GLTF!");
        return 0.0f;
    }
}

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
    this->flags = exportFlags;

    if (scene->scene == -1)
    {
        // if omitted, we can pick any scene, let's use 0
        scene->scene = 0;
    }
    // set scale
    SceneScale = scale;
    AdjustedScale = 1.0f / scale;

    size_t nodeCount = scene->nodes.Size();
    this->nodes.Reserve((SizeT)nodeCount);

    // Go through all nodes and add them to our lookup
    Util::Dictionary<Gltf::Node*, SceneNode*> nodeLookup;
    const int numRootNodes = scene->scenes[scene->scene].nodes.Size();
    
    // Traverse node hierarchies and count how many SceneNodes we will need
    int meshCounter = 0;
    for (int i = 0; i < numRootNodes; i++)
    {
        Gltf::Node* node = &scene->nodes[scene->scenes[scene->scene].nodes[i]];

        std::function<void(Gltf::Node*)> counter = [&](Gltf::Node* node)
        {
            if (node->mesh != -1)
            {
                // glTF meshes can contain several primitives, which has to be a unique SceneNode each
                // due to the fact they may vary in vertex layouts
                Gltf::Mesh* mesh = &scene->meshes[node->mesh];
                meshCounter += mesh->primitives.Size();
            }

            for (int i = 0; i < node->children.Size(); i++)
            {
                counter(&scene->nodes[node->children[i]]);
            }
        };

        counter(node);
    }
    this->meshes.Resize(meshCounter);
    // IMPORTANT: We need to preallocate all the nodes needed so that the parent/child references (pointers) don't get invalidated if the array grows.
    this->nodes.Reserve(meshCounter + scene->nodes.Size());

    for (size_t i = 0; i < numRootNodes; i++)
    {
        Gltf::Node* rootNode = &(scene->nodes[scene->scenes[scene->scene].nodes[i]]);
        ParseNodeHierarchy(scene, rootNode, nullptr, nodeLookup, this->nodes);
    }
    
    // Joint nodes are currently transform nodes since GLTF doesn't tag them as joints.
    // We need to convert them to joint nodes before continuing.

    for (size_t i = 0; i < scene->skins.Size(); i++)
    {
        Gltf::Skin const& skin = scene->skins[i];
        n_assert2(skin.inverseBindMatrices != -1, "Support for gltfs without inverse bind matrices not yet implemented!\n");

        // An accessor referenced by inverseBindMatrices MUST have floating-point components of
        // "MAT4" type.The number of elements of the accessor referenced by inverseBindMatrices MUST greater than or
        // equal to the number of joints elements. The fourth row of each matrix MUST be set to[0.0, 0.0, 0.0, 1.0].
        Gltf::Accessor const& bindAccessor = scene->accessors[skin.inverseBindMatrices];
        Gltf::BufferView const& bindBufferView = scene->bufferViews[bindAccessor.bufferView];
        Gltf::Buffer const& bindBuffer = scene->buffers[bindBufferView.buffer];
        Math::mat4 const* const buf = (Math::mat4*)((byte*)bindBuffer.data.GetPtr() + bindBufferView.byteOffset + bindAccessor.byteOffset);

        
        for (uint64_t jointIndex = 0; jointIndex < skin.joints.Size(); jointIndex++)
        {
            Math::mat4 const& inverseBindMatrix = buf[jointIndex];
            Gltf::Node* gltfJointNode = &scene->nodes[skin.joints[jointIndex]];
            SceneNode* node = nodeLookup[gltfJointNode];
            node->type = SceneNode::NodeType::Joint;
            node->anim.animIndex = 0; // default to anim 0
            node->skeleton.bindMatrix = inverseBindMatrix;
            node->skeleton.jointIndex = jointIndex;
            node->skeleton.skeletonIndex = i;
        }
    }

    // Find the root joints. Need to do this here, since we sometimes need to check if the parent is a joint or not
    // which we can't do in the above loop because all joints haven't been marked as joints yet.
    for (size_t i = 0; i < scene->skins.Size(); i++)
    {
        Gltf::Skin const& skin = scene->skins[i];
        for (auto jointNodeIndex : skin.joints)
        {
            Gltf::Node* gltfJointNode = &scene->nodes[jointNodeIndex];
            SceneNode* node = nodeLookup[gltfJointNode];
            node->skeleton.isSkeletonRoot = node->base.parent == nullptr || skin.skeleton == jointNodeIndex ||
                                            node->base.parent->type != SceneNode::NodeType::Joint;
        }
    }

    // Setup joints parent index
    for (IndexT i = 0; i < this->nodes.Size(); i++)
    {
        if (this->nodes[i].skeleton.isSkeletonRoot)
        {
            std::function<void(SceneNode*)> convertFunc = [&](SceneNode* node)
            {
                if (node->base.parent != nullptr)
                    node->skeleton.parentIndex = node->base.parent->skeleton.jointIndex;

                for (auto& child : node->base.children)
                    convertFunc(child);
            };
            convertFunc(&this->nodes[i]);
        }
    }

    this->ExtractSkeletons();

    Timing::Timer timer;

    // Extract meshes and primitives

    // Create a mapping between meshes and primitives
    IndexT basePrimitiveIndex = 0;
    std::function<void(Gltf::Node*, SceneNode*, IndexT)> nodeSetup =
        [&](Gltf::Node* gltfNode, SceneNode* parent, IndexT nodeIndex)
        {
            if (!nodeLookup.Contains(gltfNode))
                return;
            // If we encounter a mesh, we need to emit one SceneNode per mesh primitive
            SceneNode* node = nodeLookup[gltfNode];
            if (gltfNode->mesh != -1)
            {
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
                    {
                        primitiveNode->mesh.meshFlags = (MeshFlags)(primitiveNode->mesh.meshFlags | MeshFlags::HasSkin);
                        primitiveNode->base.isSkin = true;
                        primitiveNode->skeleton.skeletonIndex = gltfNode->skin; // TODO: is this right?
                    }
                }
                NglTFMesh::Setup(
                    this->meshes,
                    &scene->meshes[gltfNode->mesh],
                    scene,
                    flags,
                    basePrimitiveIndex,
                    gltfNode->mesh,
                    primitiveNodes.Begin()
                );
                basePrimitiveIndex += mesh.primitives.Size();
            }
            
            // Recurse down to children
            for (int i = 0; i < gltfNode->children.Size(); i++)
            {
                nodeSetup(&scene->nodes[gltfNode->children[i]], node, gltfNode->children[i]);
            }
        };

    // Iterate over nodes again and setup mesh and primitive nodes
    for (int i = 0; i < numRootNodes; i++)
    {
        Gltf::Node* gltfNode = &scene->nodes[scene->scenes[scene->scene].nodes[i]];
        nodeSetup(gltfNode, nullptr, scene->scenes[scene->scene].nodes[i]);
    }

    // load animations and add them to the scene animation list
    // TODO: Move to separate function
    if (!scene->animations.IsEmpty())
    {
        timer.Reset();
        timer.Start();
        AnimBuilder& animBuilder = this->animations.Emplace();
        for (auto const& animation : scene->animations)
        {
            if (animation.channels.IsEmpty())
                continue;

            AnimBuilderClip& clip = animBuilder.clips.Emplace();
            clip.SetName(animation.name);

            clip.firstCurveOffset = animBuilder.curves.Size();
            clip.numCurves = animation.channels.Size();

            for (auto const& channel : animation.channels)
            {
                AnimBuilderCurve& curve = animBuilder.curves.Emplace();
                Gltf::Animation::Sampler const& sampler = animation.samplers[channel.sampler];
                Gltf::Accessor const& curveAccessor = scene->accessors[sampler.output];
                Gltf::Accessor const& timestampAccessor = scene->accessors[sampler.input];

                int jointNode = channel.target.node;

                curve.curveType = GltfPathToCurveType(channel.target.path);
                curve.firstKeyOffset = animBuilder.keys.size();
                curve.firstTimeOffset = animBuilder.keyTimes.size();

                if (curve.curveType == CoreAnimation::CurveType::Code::Translation)
                {
                    n_assert(curveAccessor.type == Gltf::Accessor::Type::Vec3);
                }
                else if (curve.curveType == CoreAnimation::CurveType::Code::Rotation)
                {
                    // quaternion
                    n_assert(curveAccessor.type == Gltf::Accessor::Type::Vec4);
                }
                else if (curve.curveType == CoreAnimation::CurveType::Code::Scale)
                {
                    n_assert(curveAccessor.type == Gltf::Accessor::Type::Vec3);
                }
                else if (curve.curveType == CoreAnimation::CurveType::Code::Float4)
                {
                    n_assert(curveAccessor.type == Gltf::Accessor::Type::Vec4);
                }
                else
                {
                    n_error("Unsupported curve type!");
                    return;
                }

                // fill the keys and keytimes

                // GLTF doesn't define looping logic.
                curve.preInfinityType = CoreAnimation::InfinityType::Code::Cycle;
                curve.postInfinityType = CoreAnimation::InfinityType::Code::Cycle;

                curve.numKeys = curveAccessor.count;

                Gltf::BufferView const& curveBufferView = scene->bufferViews[curveAccessor.bufferView];
                Util::Blob const& curveData = scene->buffers[curveBufferView.buffer].data;
                int const curveBufferOffset = curveAccessor.byteOffset + curveBufferView.byteOffset;
                byte const* const curveBuf = (byte*)curveData.GetPtr() + curveBufferOffset;

                Gltf::BufferView const& timestampBufferView = scene->bufferViews[timestampAccessor.bufferView];
                Util::Blob const& timestampData = scene->buffers[timestampBufferView.buffer].data;
                int const timestampBufferOffset = timestampAccessor.byteOffset + timestampBufferView.byteOffset;
                byte const* const timeBuf = (byte*)timestampData.GetPtr() + timestampBufferOffset;

                // current assumption is that there are as many timestamps as keyframes. if this assert fails, we might have made the wrong assumption
                n_assert(curveAccessor.count == timestampAccessor.count);
                n_assert(timestampAccessor.type == Gltf::Accessor::Type::Scalar);

                uint64_t curveBufByteOffset = 0;
                uint64_t timestampBufByteOffset = 0;
                for (SizeT i = 0; i < curveAccessor.count; i++)
                {
                    animBuilder.keyTimes.Append(
                        ReadCurveValue(timeBuf + timestampBufByteOffset, timestampAccessor.componentType, timestampBufByteOffset) * 1000.0f
                    );

                    switch (curveAccessor.type)
                    {
                    case Gltf::Accessor::Type::Scalar:
                        animBuilder.keys.Append(ReadCurveValue(curveBuf + curveBufByteOffset, curveAccessor.componentType, curveBufByteOffset));
                        break;
                    case Gltf::Accessor::Type::Vec2:
                        animBuilder.keys.Append(ReadCurveValue(curveBuf + curveBufByteOffset, curveAccessor.componentType, curveBufByteOffset));
                        animBuilder.keys.Append(ReadCurveValue(curveBuf + curveBufByteOffset, curveAccessor.componentType, curveBufByteOffset));
                        break;
                    case Gltf::Accessor::Type::Vec3:
                        animBuilder.keys.Append(ReadCurveValue(curveBuf + curveBufByteOffset, curveAccessor.componentType, curveBufByteOffset));
                        animBuilder.keys.Append(ReadCurveValue(curveBuf + curveBufByteOffset, curveAccessor.componentType, curveBufByteOffset));
                        animBuilder.keys.Append(ReadCurveValue(curveBuf + curveBufByteOffset, curveAccessor.componentType, curveBufByteOffset));
                        break;
                    case Gltf::Accessor::Type::Vec4:
                        animBuilder.keys.Append(ReadCurveValue(curveBuf + curveBufByteOffset, curveAccessor.componentType, curveBufByteOffset));
                        animBuilder.keys.Append(ReadCurveValue(curveBuf + curveBufByteOffset, curveAccessor.componentType, curveBufByteOffset));
                        animBuilder.keys.Append(ReadCurveValue(curveBuf + curveBufByteOffset, curveAccessor.componentType, curveBufByteOffset));
                        animBuilder.keys.Append(ReadCurveValue(curveBuf + curveBufByteOffset, curveAccessor.componentType, curveBufByteOffset));
                        break;
                    default:
                        break;
                    }
                }
                // TODO: Double check that this is right.
                clip.duration = animBuilder.keyTimes.Back() - animBuilder.keyTimes[curve.firstTimeOffset];
            }
        }
        timer.Stop();
        n_printf("    [glTF - Animations read (%d, %d ms)]\n", scene->animations.Size(), timer.GetTime() * 1000);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
NglTFScene::ParseNodeHierarchy(
    Gltf::Document* scene,
    Gltf::Node* gltfNode,
    SceneNode* parent,
    Util::Dictionary<Gltf::Node*, SceneNode*>& lookup,
    Util::Array<SceneNode>& nodes
)
{
    SceneNode& node = nodes.Emplace();
    n_assert(!lookup.Contains(gltfNode));
    lookup.Add(gltfNode, &node);
    
    // Temporarily set all nodes to be transforms.
    // We will later convert some of them to joints as we can't
    // do it here since GLTF doesn't readily keep track of it.
    // We will also generate additional nodes that will act as meshes, since GLTF
    // has support for different vertex layouts for primitives within the same
    // mesh, and we don't. Each GLTF primitive becomes a Nebula mesh.
    node.Setup(SceneNode::NodeType::Transform);
    NglTFNode::Setup(gltfNode, &node, parent);

    node.base.name = gltfNode->name;
    if (node.base.name.IsEmpty())
    {
        node.base.name = UniqueString::New("unnamed");
    }

    size_t numChildren = gltfNode->children.Size();
    for (size_t i = 0; i < numChildren; i++)
    {
        Gltf::Node* child = &scene->nodes[gltfNode->children[i]];
        ParseNodeHierarchy(scene, child, &node, lookup, nodes);
    }
}

} // namespace ToolkitUtil
