//------------------------------------------------------------------------------
//  fbxlightnode.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "nfbxnode.h"
#include "nfbxlightnode.h"
#include "model/import/base/scenenode.h"

namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
void 
NFbxLightNode::Setup(SceneNode* node, SceneNode* parent, FbxNode* fbxNode)
{
    NFbxNode::Setup(node, parent, fbxNode);
    FbxLight* light = fbxNode->GetLight();
    switch (light->LightType)
    {
        case FbxLight::ePoint:
            node->light.lightType = SceneNode::Pointlight;
            break;
        case FbxLight::eSpot:
            node->light.lightType = SceneNode::Spotlight;
            break;
        case FbxLight::eArea:
            node->light.lightType = SceneNode::AreaLight;
            break;
        default:
            node->light.lightType = SceneNode::InvalidLight;
            break;
    }
}


} // namespace ToolkitUtil