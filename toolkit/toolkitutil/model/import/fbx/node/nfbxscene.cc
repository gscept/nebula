//------------------------------------------------------------------------------
//  fbxnoderegistry.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "nfbxscene.h"
#include "model/import/fbx/nfbxexporter.h"
#include "model/skeletonutil/skeletonbuilder.h"
#include "nfbxmeshnode.h"
#include "nfbxjointnode.h"
#include "nfbxtransformnode.h"
#include "nfbxlightnode.h"

#include <fbxsdk.h>

using namespace Util;
using namespace ToolkitUtil;
namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
NFbxScene::NFbxScene()
{
}

//------------------------------------------------------------------------------
/**
*/
NFbxScene::~NFbxScene()
{
}

//------------------------------------------------------------------------------
/**
*/
void
NFbxScene::ParseNodeHierarchy(
    FbxNode* fbxNode
    , SceneNode* parent
    , Util::Dictionary<FbxNode*, SceneNode*>& lookup
    , Util::Array<SceneNode>& nodes
)
{
    SceneNode& node = nodes.Emplace();
    FbxTransform::EInheritType inherit;
    //fbxNode->GetTransformationInheritType(inherit);
    //n_assert(inherit == FbxTransform::EInheritType::eInheritRrs);
    lookup.Add(fbxNode, &node);
    const FbxNodeAttribute* attr = fbxNode->GetNodeAttribute();
    const FbxNodeAttribute::EType type = attr == nullptr ? FbxNodeAttribute::EType::eNull : attr->GetAttributeType();
    switch (type)
    {
        case FbxNodeAttribute::EType::eSkeleton:
        {
            node.Setup(SceneNode::NodeType::Joint);
            NFbxJointNode::Setup(&node, parent, fbxNode);
            break;
        }
        case FbxNodeAttribute::EType::eMesh:
        {
            node.Setup(SceneNode::NodeType::Mesh);
            NFbxNode::Setup(&node, parent, fbxNode);
            break;
        }
        case FbxNodeAttribute::EType::eLODGroup:
        {
            node.Setup(SceneNode::NodeType::Lod);
            NFbxNode::Setup(&node, parent, fbxNode);
            break;
        }
        case FbxNodeAttribute::EType::eLight:
        {
            node.Setup(SceneNode::NodeType::Light);
            NFbxLightNode::Setup(&node, parent, fbxNode);
            break;
        }
        default:
        {
            node.Setup(SceneNode::NodeType::Transform);
            NFbxNode::Setup(&node, parent, fbxNode);
            break;
        }
    }

    int numChildren = fbxNode->GetChildCount();
    for (int i = 0; i < numChildren; i++)
    {
        FbxNode* child = fbxNode->GetChild(i);
        ParseNodeHierarchy(child, &node, lookup, nodes);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ResetNodePivots(FbxNode* fbxNode)
{
    FbxVector4 lZero(0, 0, 0);

    // Activate pivot converting
    fbxNode->SetPivotState(FbxNode::eSourcePivot, FbxNode::ePivotActive);
    fbxNode->SetPivotState(FbxNode::eDestinationPivot, FbxNode::ePivotActive);

    // We want to set all these to 0 and bake them into the transforms.
    fbxNode->SetPostRotation(FbxNode::eDestinationPivot, lZero);
    fbxNode->SetPreRotation(FbxNode::eDestinationPivot, lZero);
    fbxNode->SetRotationOffset(FbxNode::eDestinationPivot, lZero);
    fbxNode->SetScalingOffset(FbxNode::eDestinationPivot, lZero);
    fbxNode->SetRotationPivot(FbxNode::eDestinationPivot, lZero);
    fbxNode->SetScalingPivot(FbxNode::eDestinationPivot, lZero);

    // This is to import in a system that supports rotation order.
    // If rotation order is not supported, do this instead:
    // pNode->SetRotationOrder(FbxNode::eDESTINATION_SET , FbxNode::eEULER_XYZ);
    FbxEuler::EOrder lRotationOrder(FbxEuler::eOrderXYZ);
    fbxNode->GetRotationOrder(FbxNode::eSourcePivot, lRotationOrder);
    fbxNode->SetRotationOrder(FbxNode::eDestinationPivot, lRotationOrder);

    // Similarly, this is the case where geometric transforms are supported by the system.
    // If geometric transforms are not supported, set them to zero instead of
    // the source’s geometric transforms.
    // Geometric transform = local transform, not inherited by children.
    fbxNode->SetGeometricTranslation(FbxNode::eDestinationPivot, fbxNode->GetGeometricTranslation(FbxNode::eSourcePivot));
    fbxNode->SetGeometricRotation(FbxNode::eDestinationPivot, fbxNode->GetGeometricRotation(FbxNode::eSourcePivot));
    fbxNode->SetGeometricScaling(FbxNode::eDestinationPivot, fbxNode->GetGeometricScaling(FbxNode::eSourcePivot));

    fbxNode->SetQuaternionInterpolation(FbxNode::eDestinationPivot, fbxNode->GetQuaternionInterpolation(FbxNode::eSourcePivot));
}

//------------------------------------------------------------------------------
/**
*/
void
NFbxScene::Setup(
    FbxScene* scene
    , const ExportFlags& exportFlags
    , const Ptr<ModelAttributes>& attributes
    , float scale
)
{
    n_assert(scene);

    // set export settings
    this->flags = exportFlags;

    // set scale
    SceneScale = scale;
    AdjustedScale = SceneScale;

    float fps = TimeModeToFPS(scene->GetGlobalSettings().GetTimeMode());

    // handle special case for custom FPS
    if (fps == -1)
    {
        fps = (float)scene->GetGlobalSettings().GetCustomFrameRate();
    }
    scene->GetRootNode()->ResetPivotSetAndConvertAnimation(fps);
    AnimationFrameRate = fps;

    // split meshes based on material
    FbxGeometryConverter* converter = new FbxGeometryConverter(sdkManager);
    bool triangulated = converter->Triangulate(scene, true);
    converter->RemoveBadPolygonsFromMeshes(scene);
    n_assert(triangulated);
    delete converter;

    // Okay so we want to do this, we really do, but if we do, 
    // the GetSrcObjectCount will give us an INCORRECT amount of meshes, 
    // and getting the mesh will give us invalid meshes...
    //bool foo = converter->SplitMeshesPerMaterial(scene, true);
    //delete converter;
        
    // Get number of meshes
    int nodeCount = scene->GetSrcObjectCount<FbxNode>();
    this->nodes.Reserve(nodeCount);

    // Go through all nodes and add them to our lookup
    Util::Dictionary<FbxNode*, SceneNode*> nodeLookup;
    ParseNodeHierarchy(scene->GetRootNode(), nullptr, nodeLookup, this->nodes);

    // Extract data from nodes
    for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
    {
        SceneNode* node = &this->nodes[nodeIndex];
        switch (node->type)
        {
            case SceneNode::NodeType::Mesh:
                NFbxMeshNode::ExtractMesh(node, this->meshes, nodeLookup, flags);
                break;
        }
    }

    // Extract animation curves
    int animStackCount = scene->GetSrcObjectCount<FbxAnimStack>();
    for (int animStackIndex = 0; animStackIndex < animStackCount; animStackIndex++)
    {
        FbxAnimStack* animStack = scene->GetSrcObject<FbxAnimStack>(animStackIndex);
        scene->SetCurrentAnimationStack(animStack);
        for (int nodeIndex = 0; nodeIndex < this->nodes.Size(); nodeIndex++)
        {
            SceneNode* node = &this->nodes[nodeIndex];
            NFbxNode::PrepareAnimation(node, animStack);
            if (node->base.isAnimated && (node->base.parent == nullptr || !node->base.parent->base.isAnimated))
            {
                node->anim.animIndex = this->animations.Size();

                AnimBuilder& anim = this->animations.Emplace();

                // Extract animation keys from the node hierarchy
                NFbxNode::ExtractAnimation(node, anim.keys, anim.keyTimes, animStack);

                // Generate one clip per anim stack
                Scene::GenerateClip(node, anim, animStack->GetName());
            }
        }
    }

    // Extract skeletons
    this->ExtractSkeletons();
}

//------------------------------------------------------------------------------
/**
*/
void
NFbxScene::ExtractSkeletons()
{
    Util::Array<SceneNode*> skeletonRoots;
    for (IndexT i = 0; i < this->nodes.Size(); i++)
    {
        if (this->nodes[i].skeleton.isSkeletonRoot)
        {
            // Associate this node with the skeleton resource
            this->nodes[i].skeleton.skeletonIndex = skeletonRoots.Size();
            skeletonRoots.Append(&this->nodes[i]);
        }
    }

    this->skeletons.Resize(skeletonRoots.Size());
    for (IndexT i = 0; i < this->skeletons.Size(); i++)
    {
        SkeletonBuilder& builder = this->skeletons[i];
        std::function<void(SceneNode* node, SkeletonBuilder&)> convertFunc = [&](SceneNode* node, SkeletonBuilder& builder)
        {
            if (node->base.parent != nullptr)
                node->skeleton.parentIndex = node->base.parent->skeleton.jointIndex;

            Joint joint;
            joint.name = node->base.name;
            joint.bind = node->skeleton.bindMatrix;
            joint.translation = node->base.translation;
            joint.rotation = node->base.rotation;
            joint.scale = node->base.scale;
            joint.index = node->skeleton.jointIndex;
            joint.parent = node->skeleton.parentIndex;
            builder.joints.Append(joint);

            for (auto& child : node->base.children)
                convertFunc(child, builder);
        };
        convertFunc(skeletonRoots[i], builder);
    }
}

//------------------------------------------------------------------------------
/**
*/
float
NFbxScene::TimeModeToFPS(const FbxTime::EMode& timeMode)
{
    switch (timeMode)
    {
        case FbxTime::eFrames100: return 100;
        case FbxTime::eFrames120: return 120;
        case FbxTime::eFrames1000: return 1000;
        case FbxTime::eFrames30: return 30;
        case FbxTime::eFrames30Drop: return 30;
        case FbxTime::eFrames48: return 48;
        case FbxTime::eFrames50: return 50;
        case FbxTime::eFrames60: return 60;
        case FbxTime::eNTSCDropFrame: return 29.97002617f;
        case FbxTime::eNTSCFullFrame: return 29.97002617f;
        case FbxTime::ePAL: return 25;
        case FbxTime::eCustom: return -1; // invalid mode, has to be handled separately
        default: return 24;
    }
}

} // namespace ToolkitUtil