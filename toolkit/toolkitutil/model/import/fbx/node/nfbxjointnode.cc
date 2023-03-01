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
NFbxJointNode::Setup(SceneNode* node, SceneNode* parent, FbxNode* fbxNode)
{
    NFbxNode::Setup(node, parent, fbxNode);
    FbxSkeleton* joint = fbxNode->GetSkeleton();
    node->skeleton.isSkeletonRoot = joint->IsSkeletonRoot();
}

} // namespace ToolkitUtil