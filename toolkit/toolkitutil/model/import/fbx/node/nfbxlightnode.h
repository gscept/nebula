#pragma once
//------------------------------------------------------------------------------
/**
    @class ToolkitUtil::FbxLightNode
    
    Represents an FBX light as a nebula light
    
    (C) 2022 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include <fbxsdk.h>
#include "model/import/base/scenenode.h"
namespace ToolkitUtil
{
class SceneNode;
class NFbxLightNode
{
public:
    /// Setup node from FBX node
    static void Setup(SceneNode* node, SceneNode* parent, ufbx_node* fbxNode);

    enum LightType
    {
        Spotlight,
        Pointlight,
        AreaLight,
        InvalidLight,

        NumLights
    };
}; 
} // namespace ToolkitUtil
//------------------------------------------------------------------------------