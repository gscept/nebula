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
#include "timing/timer.h"

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
NFbxScene::Setup(
    FbxScene* scene
    , const ExportFlags& exportFlags
    , const Ptr<ModelAttributes>& attributes
    , float scale
    , ToolkitUtil::Logger* logger
)
{
    n_assert(scene);

    // set export settings
    this->flags = exportFlags;
    this->logger = logger;

    // set scale
    SceneScale = scale;
    AdjustedScale = SceneScale;

    float fps = TimeModeToFPS(scene->GetGlobalSettings().GetTimeMode());

    auto axisSystem = scene->GetGlobalSettings().GetAxisSystem();
    FbxAxisSystem newSystem(FbxAxisSystem::eOpenGL);
    if (axisSystem != newSystem)
    {
        newSystem.DeepConvertScene(scene);
        this->logger->Print("FBX - Converting coordinate system\n");

    }

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

    this->logger->Print("FBX - Triangulating\n");


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

    // Setup skeleton hierarchy
    this->SetupSkeletons();

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

    // Update the skeleton builder
    this->ExtractSkeletons();

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