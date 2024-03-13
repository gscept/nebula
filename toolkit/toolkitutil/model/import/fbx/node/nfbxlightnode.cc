//------------------------------------------------------------------------------
//  fbxlightnode.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "foundation/stdneb.h"
#include "nfbxnode.h"
#include "nfbxlightnode.h"
#include "model/import/base/scenenode.h"
#include "ufbx/ufbx.h"

namespace ToolkitUtil
{

//------------------------------------------------------------------------------
/**
*/
void 
NFbxLightNode::Setup(SceneNode* node, SceneNode* parent, ufbx_node* fbxNode)
{
    NFbxNode::Setup(node, parent, fbxNode);
    ufbx_light* light = fbxNode->light;
    switch (light->type)
    {
        case UFBX_LIGHT_POINT:
            node->light.lightType = SceneNode::Pointlight;
            break;
        case UFBX_LIGHT_SPOT:
            node->light.lightType = SceneNode::Spotlight;
            break;
        case UFBX_LIGHT_AREA:
            node->light.lightType = SceneNode::AreaLight;
            break;
        default:
            node->light.lightType = SceneNode::InvalidLight;
            break;
    }
}


} // namespace ToolkitUtil