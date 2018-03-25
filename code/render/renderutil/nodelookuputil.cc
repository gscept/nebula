//------------------------------------------------------------------------------
//  nodelookuputil.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "renderutil/nodelookuputil.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/graphicsentity.h"
#include "models/model.h"

namespace RenderUtil
{
using namespace Models;
using namespace Graphics;

Util::String NodeLookupUtil::currentNodeLayer;
//------------------------------------------------------------------------------
/**
    FIXME: THESE METHODS ARE SLOW AS HELL!!!

    Utility function which searches a specified statenodeinstance in a 
    graphicsentity.

    returns 0 if 
        - resource is not loaded and resourcecheck flag is enabled
        - node was not found
        - modelentity is not active
        - model instances is not valid
*/
Models::ShaderStateNode::Instance* LookupStateNodeInstance(const GraphicsEntityId& entity,
	const Util::String& nodeName,
	bool checkResourceState/* = true*/)
{
    return nullptr;
}

} // namespace RenderUtil
