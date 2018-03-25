//------------------------------------------------------------------------------
//  fbxlightnode.cc
//  (C) 2012-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "fbx/node/nfbxlightnode.h"

namespace ToolkitUtil
{
__ImplementClass(ToolkitUtil::NFbxLightNode, 'FBLN', ToolkitUtil::NFbxNode);

//------------------------------------------------------------------------------
/**
*/
NFbxLightNode::NFbxLightNode()
{
	this->type = NFbxNode::Light;
}

//------------------------------------------------------------------------------
/**
*/
NFbxLightNode::~NFbxLightNode()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void 
NFbxLightNode::Setup( FbxNode* node, const Ptr<NFbxScene>& scene )
{
	NFbxNode::Setup(node, scene);
	n_assert(node->GetLight());
	this->light = node->GetLight();

	switch (this->light->LightType)
	{
	case FbxLight::ePoint:
		this->lightType = Pointlight;
		break;
	case FbxLight::eSpot:
		this->lightType = Spotlight;
		break;
	case FbxLight::eArea:
		this->lightType = AreaLight;
		break;
	default:
		this->lightType = InvalidLight;
		break;		
	}
}
} // namespace ToolkitUtil