//------------------------------------------------------------------------------
//  fbxjointnode.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "nfbxjointnode.h"
#include "nfbxscene.h"
#include "math/mat4.h"
#include "math/vec4.h"
#include "nfbxnode.h"

using namespace Math;
namespace ToolkitUtil
{

int JointCounter = 0;
//------------------------------------------------------------------------------
/**
*/
void 
NFbxJointNode::Setup(SceneNode* node, SceneNode* parent, ufbx_node* fbxNode)
{
    NFbxNode::Setup(node, parent, fbxNode);
    ufbx_bone* bone = ufbx_as_bone(fbxNode->attrib);
    ufbx_node* parentFbx = fbxNode->parent;
    node->skeleton.isSkeletonRoot = parent == nullptr || parentFbx->attrib_type == UFBX_ELEMENT_UNKNOWN;
}

} // namespace ToolkitUtil