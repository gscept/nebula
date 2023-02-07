#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxJointNode
    
    Wraps an FBX skeleton node as a Nebula-style node
    
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <fbxsdk.h>
#include "model/import/base/scenenode.h"
namespace ToolkitUtil
{
class SceneNode;
class NFbxJointNode
{
public:
    /// Setup node from FBX node
    static void Setup(SceneNode* node, FbxNode* fbxNode, FbxPose* bindpose);
}; 

} // namespace ToolkitUtil
//------------------------------------------------------------------------------