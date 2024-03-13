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
#include "nfbxlightnode.h"
#include "timing/timer.h"

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
    ufbx_node* fbxNode
    , SceneNode* parent
    , Util::Dictionary<ufbx_node*, SceneNode*>& lookup
    , Util::Array<SceneNode>& nodes
)
{
    SceneNode& node = nodes.Emplace();
    lookup.Add(fbxNode, &node);
    ufbx_element_type attr = fbxNode->attrib_type;
    switch (attr)
    {
        case UFBX_ELEMENT_BONE:
        {
            node.Setup(SceneNode::NodeType::Joint);
            NFbxJointNode::Setup(&node, parent, fbxNode); 
            break;
        }
        case UFBX_ELEMENT_MESH:
        {
            node.Setup(SceneNode::NodeType::Mesh);
            NFbxNode::Setup(&node, parent, fbxNode);
            break;
        }
        case UFBX_ELEMENT_LOD_GROUP:
        {
            node.Setup(SceneNode::NodeType::Lod);
            NFbxNode::Setup(&node, parent, fbxNode);
            break;
        }
        case UFBX_ELEMENT_LIGHT:
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

    size_t numChildren = fbxNode->children.count;
    for (size_t i = 0; i < numChildren; i++)
    {
        ufbx_node* child = fbxNode->children[i];
        ParseNodeHierarchy(child, &node, lookup, nodes);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
NFbxScene::Setup(
    ufbx_scene* scene
    , const ExportFlags& exportFlags
    , const Ptr<ModelAttributes>& attributes
    , float scale
    , ToolkitUtil::Logger* logger
)
{
    n_assert(scene != nullptr);

    // set export settings
    this->flags = exportFlags;
    this->logger = logger;

    // set scale
    SceneScale = scale;
    AdjustedScale = SceneScale;

    
    float fps = TimeModeToFPS(scene->settings.time_mode);
    // handle special case for custom FPS
    if (fps == -1)
    {
        fps = scene->settings.frames_per_second;
    }
    AnimationFrameRate = fps;

    // Get number of meshes
    size_t nodeCount = scene->nodes.count;
    this->nodes.Reserve((SizeT)nodeCount);

    // Go through all nodes and add them to our lookup
    Util::Dictionary<ufbx_node*, SceneNode*> nodeLookup;
    ParseNodeHierarchy(scene->root_node, nullptr, nodeLookup, this->nodes);

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
    size_t animStackCount = scene->anim_stacks.count;
    for (size_t animStackIndex = 0; animStackIndex < animStackCount; animStackIndex++)
    {
        ufbx_anim_stack* animStack = scene->anim_stacks[animStackIndex];
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
                Scene::GenerateClip(node, anim, Util::String(animStack->name.data, animStack->name.length));
            }
        }
    }
}

//------------------------------------------------------------------------------
/**
*/
float
NFbxScene::TimeModeToFPS(const ufbx_time_mode timeMode)
{
    switch (timeMode)
    {
        case UFBX_TIME_MODE_100_FPS: return 100;
        case UFBX_TIME_MODE_120_FPS: return 120;
        case UFBX_TIME_MODE_1000_FPS: return 1000;
        case UFBX_TIME_MODE_30_FPS: return 30;
        case UFBX_TIME_MODE_30_FPS_DROP: return 30;
        case UFBX_TIME_MODE_48_FPS: return 48;
        case UFBX_TIME_MODE_50_FPS: return 50;
        case UFBX_TIME_MODE_60_FPS: return 60;
        case UFBX_TIME_MODE_NTSC_DROP_FRAME: return 29.97002617f;
        case UFBX_TIME_MODE_NTSC_FULL_FRAME: return 29.97002617f;
        case UFBX_TIME_MODE_PAL: return 25;
        case UFBX_TIME_MODE_CUSTOM: return -1; // invalid mode, has to be handled separately
        default: return 24;
    }
}

} // namespace ToolkitUtil