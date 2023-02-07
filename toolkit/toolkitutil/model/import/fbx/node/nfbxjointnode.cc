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

uint childCounter = 0;
//------------------------------------------------------------------------------
/**
*/
void 
NFbxJointNode::Setup(SceneNode* node, FbxNode* fbxNode, FbxPose* bindpose)
{
    NFbxNode::Setup(node, fbxNode);
    FbxSkeleton* joint = fbxNode->GetSkeleton();
    node->skeleton.isSkeletonRoot = joint->IsSkeletonRoot();

    if (node->base.parent != nullptr && node->base.parent->type == SceneNode::NodeType::Joint)
    {
        node->skeleton.parentIndex = node->base.parent->skeleton.jointIndex;
        node->skeleton.jointIndex = ++childCounter;
    }
    else
    {
        node->skeleton.jointIndex = 0;
        childCounter = 0;
    }

    if (bindpose)
    {
        IndexT idx = bindpose->Find(fbxNode->GetName());
        node->skeleton.globalMatrix = FbxToMath(bindpose->GetMatrix(idx));
        node->skeleton.matrixIsGlobal = true;
    }
}

} // namespace ToolkitUtil