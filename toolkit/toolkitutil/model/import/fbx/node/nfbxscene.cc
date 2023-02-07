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
NFbxScene::Setup(
    FbxScene* scene
    , const ExportFlags& exportFlags
    , const Ptr<ModelAttributes>& attributes
    , float scale
)
{
    n_assert(scene);

    // create scene mesh
    SceneScale = scale;
    AdjustedScale = SceneScale * 1 / float(scene->GetGlobalSettings().GetSystemUnit().GetScaleFactor());

    // set export settings
    this->flags = exportFlags;

    // set scale
    this->scale = scale;

    float fps = TimeModeToFPS(scene->GetGlobalSettings().GetTimeMode());

    // handle special case for custom FPS
    if (fps == -1)
    {
        fps = (float)scene->GetGlobalSettings().GetCustomFrameRate();
    }
    scene->GetRootNode()->ResetPivotSetAndConvertAnimation(fps, true);

    // split meshes based on material
    FbxGeometryConverter* converter = new FbxGeometryConverter(sdkManager);
    bool triangulated = converter->Triangulate(scene, true);
    delete converter;

    // Okay so we want to do this, we really do, but if we do, 
    // the GetSrcObjectCount will give us an INCORRECT amount of meshes, 
    // and getting the mesh will give us invalid meshes...
    //bool foo = converter->SplitMeshesPerMaterial(scene, true);
    //delete converter;
        
    // Get number of meshes
    int nodeCount = scene->GetSrcObjectCount<FbxNode>();
    this->nodes.Resize(nodeCount);

    // Go through all nodes and add them to our lookup
    Util::Dictionary<FbxNode*, SceneNode*> nodeLookup;
    for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
    {
        FbxNode* fbxNode = scene->GetSrcObject<FbxNode>(nodeIndex);
        nodeLookup.Add(fbxNode, &this->nodes[nodeIndex]);
    }

    // Add children to parents
    for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
    {
        FbxNode* fbxNode = scene->GetSrcObject<FbxNode>(nodeIndex);
        FbxNode* fbxParentNode = fbxNode->GetParent();
        
        if (fbxParentNode != nullptr)
        {
            SceneNode& node = this->nodes[nodeIndex];
            SceneNode* parent = nodeLookup[fbxParentNode];
            n_assert(parent != nullptr);
            parent->base.children.Append(&node);
            node.base.parent = parent;
        }
    }

    FbxPose* bindpose = nullptr;
    int poses = scene->GetPoseCount();
    for (int i = 0; i < poses; i++)
    {
        FbxPose* pose = scene->GetPose(i);
        if (pose->IsBindPose())
        {
            bindpose = pose;
            break;
        }
    }

    // Extract data from nodes
    for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
    {
        FbxNode* fbxNode = scene->GetSrcObject<FbxNode>(nodeIndex);
        SceneNode& node = this->nodes[nodeIndex];
        const FbxNodeAttribute* attr = fbxNode->GetNodeAttribute();
        const FbxNodeAttribute::EType type = attr == nullptr ? FbxNodeAttribute::EType::eNull : attr->GetAttributeType();
        switch (type)
        {
            case FbxNodeAttribute::EType::eSkeleton:
            {
                node.Setup(SceneNode::NodeType::Joint);
                NFbxJointNode::Setup(&node, fbxNode, bindpose);
                break;
            }
            case FbxNodeAttribute::EType::eMesh:
            {
                node.Setup(SceneNode::NodeType::Mesh);
                NFbxMeshNode::Setup(&node, fbxNode);
                break;
            }
            case FbxNodeAttribute::EType::eLODGroup:
            {
                node.Setup(SceneNode::NodeType::Lod);
                NFbxNode::Setup(&node, fbxNode);
                break;
            }
            case FbxNodeAttribute::EType::eLight:
            {
                node.Setup(SceneNode::NodeType::Light);
                NFbxLightNode::Setup(&node, fbxNode);
                break;
            }
            default:
            {
                node.Setup(SceneNode::NodeType::Transform);
                NFbxNode::Setup(&node, fbxNode);
                break;
            }
        }
    }

    // Extract data from nodes
    for (int nodeIndex = 0; nodeIndex < nodeCount; nodeIndex++)
    {
        FbxNode* fbxNode = scene->GetSrcObject<FbxNode>(nodeIndex);
        SceneNode& node = this->nodes[nodeIndex];
        switch (node.type)
        {
            case SceneNode::NodeType::Mesh:
                NFbxMeshNode::ExtractMesh(&node, this->meshes, nodeLookup, fbxNode, flags);
                break;
        }
    }

    // Extract animation curves
    int animStackCount = scene->GetSrcObjectCount<FbxAnimStack>();
    for (int animStackIndex = 0; animStackIndex < animStackCount; animStackIndex++)
    {
        FbxAnimStack* animStack = scene->GetSrcObject<FbxAnimStack>(animStackIndex);
        for (int nodeIndex = 0; nodeIndex < this->nodes.Size(); nodeIndex++)
        {
            FbxNode* fbxNode = scene->GetSrcObject<FbxNode>(nodeIndex);
            SceneNode* node = &this->nodes[nodeIndex];
            NFbxNode::ExtractAnimation(node, fbxNode, animStack);
        }
    }

    // Extract animations
    this->ExtractAnimations(attributes);

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
        if (this->nodes[i].type == SceneNode::NodeType::Joint)
        {
            if (this->nodes[i].base.parent == nullptr
                || this->nodes[i].base.parent->type != SceneNode::NodeType::Joint)
            {
                // Associate this node with the skeleton resource
                this->nodes[i].skeleton.skeletonIndex = skeletonRoots.Size();
                skeletonRoots.Append(&this->nodes[i]);
            }
        }
    }

    this->skeletons.Resize(skeletonRoots.Size());
    for (IndexT i = 0; i < this->skeletons.Size(); i++)
    {
        SkeletonBuilder& builder = this->skeletons[i];
        uint index = 0;
        std::function<void(SceneNode* node, SkeletonBuilder&, uint&)> convertFunc = [&](SceneNode* node, SkeletonBuilder& builder, uint& index)
        {
            if (node->base.parent != nullptr)
                node->skeleton.parentIndex = node->base.parent->skeleton.jointIndex;

            Joint joint;
            joint.name = node->base.name;
            joint.translation = node->base.position;
            joint.rotation = node->base.rotation;
            joint.scale = node->base.scale;
            joint.index = node->skeleton.jointIndex;
            joint.parent = node->skeleton.parentIndex;
            builder.joints.Append(joint);

            for (auto& child : node->base.children)
                convertFunc(child, builder, index);
        };
        convertFunc(skeletonRoots[i], builder, index);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
NFbxScene::ExtractAnimations(const Ptr<ToolkitUtil::ModelAttributes>& attributes)
{
    SizeT numAnims = 0;
    Util::Array<SceneNode*> animatedRootNodes;
    for (IndexT i = 0; i < this->nodes.Size(); i++)
    {
        if (this->nodes[i].base.isAnimated
            && this->nodes[i].type == SceneNode::NodeType::Joint
            && (this->nodes[i].base.parent == nullptr
            || this->nodes[i].base.parent->base.isAnimated == false))
        {
            // Associate this node with the animation subresource
            this->nodes[i].anim.animIndex = animatedRootNodes.Size();
            animatedRootNodes.Append(&this->nodes[i]);
        }
    }

    this->animations.Resize(animatedRootNodes.Size());

    for (IndexT i = 0; i < animatedRootNodes.Size(); i++)
    {
        AnimBuilder& anim = this->animations[i];
        SceneNode* node = animatedRootNodes[i];
        Util::Array<AnimBuilderCurve> curves;
        curves.Append(node->anim.translationCurve);
        curves.Append(node->anim.rotationCurve);
        curves.Append(node->anim.scaleCurve);

        // If we have no split rules, create a single clip for the curves
        if (!SplitAnimationCurves(animatedRootNodes[i], curves, anim, attributes))
        {
            // If split fails, just add the whole animation
            AnimBuilderClip clip;

            int numCurves = curves.Size();
            int keyCount = 0;

            for (int curveIndex = 0; curveIndex < numCurves; curveIndex++)
            {
                clip.AddCurve(curves[curveIndex]);
                keyCount = Math::max(keyCount, curves[curveIndex].GetNumKeys());
            }

            clip.SetName(name);
            clip.SetNumKeys(keyCount);
            clip.SetKeyDuration(KeysPerMS);
            clip.SetStartKeyIndex(0);
            clip.SetPreInfinityType(node->anim.preInfinity);
            clip.SetPostInfinityType(node->anim.postInfinity);
            anim.AddClip(clip);
        }
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