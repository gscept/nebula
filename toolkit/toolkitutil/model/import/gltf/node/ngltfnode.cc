//------------------------------------------------------------------------------
//  ngltfnode.h
//  (C) 2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "ngltfnode.h"
#include "ngltfscene.h"
#include "model/modelutil/clip.h"
#include "model/modelutil/clipevent.h"
#include "model/modelutil/take.h"

namespace ToolkitUtil
{

using namespace Math;
using namespace CoreAnimation;
using namespace Util;
using namespace ToolkitUtil;

__ImplementClass(ToolkitUtil::NglTFNode, 'ASNO', Core::RefCounted);

//------------------------------------------------------------------------------
/**
*/
NglTFNode::NglTFNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
NglTFNode::~NglTFNode()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void 
NglTFNode::Setup(const Gltf::Node* gltfNode, SceneNode* node, SceneNode* parent)
{
    if (node->base.name == "physics")
    {
        node->base.isPhysics = true;
    }

    if (parent != nullptr)
    {
        node->base.parent = parent;
        parent->base.children.Append(node);
    }

    // construct nebula matrix
    float scaleFactor = SceneScale;

    if (gltfNode->hasTRS)
    {
        node->base.translation = gltfNode->translation;
        node->base.rotation = gltfNode->rotation;
        node->base.scale = gltfNode->scale;
    }
    else
    {
        decompose(gltfNode->matrix, node->base.scale, node->base.rotation, node->base.translation);
        node->base.translation *= scaleFactor;
    }
    
}

} // namespace ToolkitUtil