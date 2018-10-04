#pragma once
//------------------------------------------------------------------------------
/**
    @class RenderUtil::NodeLookupUtil

    Helper class to find specific nodes and nodeinstances inside a graphicsentity

    WARNING: this util uses SLOW methods, like 'ModelInstance::LookupNodeInstance',
             use it careful!!!

    (C) 2009 Radon Labs GmbH
    (C) 2013-2018 Individual contributors, see AUTHORS file
*/
#include "util/stringatom.h"
#include "models/nodes/shaderstatenode.h"
#include "models/modelcontext.h"
#include "models/nodes/modelnode.h"
#include "resources/resourceid.h"

//------------------------------------------------------------------------------
namespace RenderUtil
{
class NodeLookupUtil
{
public:
    /// find state node instance
    static Models::ShaderStateNode::Instance* LookupStateNodeInstance(const GraphicsEntityId& entity,
                                                                  const Util::String& nodeName, 
                                                                  bool checkResourceState = true);

private:
	// gets the names of modelNode and all its children's children children children	
	static Util::Array<Util::String> RecursiveGetNodeNames(const Ptr<Models::ModelNode>& modelNode /*rootNode*/, const Util::FourCC & fourcc);
	static Util::String currentNodeLayer;
};

} // namespace RenderUtil
//------------------------------------------------------------------------------

