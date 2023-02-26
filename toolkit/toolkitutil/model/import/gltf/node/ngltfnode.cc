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
NglTFNode::Setup(const Gltf::Node* gltfNode, SceneNode* node)
{
    node->base.name = gltfNode->name;
    if (node->base.name == "physics")
    {
        node->base.isPhysics = true;
    }

    node->mesh.lodIndex = InvalidIndex;
    node->mesh.meshIndex = InvalidIndex;
    node->skeleton.skeletonIndex = InvalidIndex;
    node->anim.animIndex = InvalidIndex;

    // construct nebula matrix
    float scaleFactor = SceneScale;

    if (gltfNode->hasTRS)
    {
        node->base.translation = gltfNode->translation;
        //node->base.rotation = gltfNode->rotation;
        node->base.scale = gltfNode->scale;
    }
    else
    {
        Math::quat rotation;
        Math::vec3 translation;
        Math::vec3 scale;
        double sign;

        decompose(gltfNode->matrix, scale, rotation, translation);
        sign = determinant(gltfNode->matrix);
        //node->base.rotation = rotation;
        node->base.translation = vec3(translation[0] * scaleFactor, translation[1] * scaleFactor, translation[2] * scaleFactor);
        node->base.scale = vec3(scale[0], scale[1], scale[2]);
    }
    
}

} // namespace ToolkitUtil