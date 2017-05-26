#pragma once
//------------------------------------------------------------------------------
/**
    @class RenderUtil::NodeLookupUtil

    Helper class to find specific nodes and nodeinstances inside a graphicsentity

    WARNING: this util uses SLOW methods, like 'ModelInstance::LookupNodeInstance',
             use it careful!!!

    (C) 2009 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#include "util/stringatom.h"
#include "models/nodes/statenodeinstance.h"
#include "models/nodes/animatornodeinstance.h"
#include "models/nodes/statenodeinstance.h"
#include "graphics/modelentity.h"
#include "models/modelnode.h"
#include "resources/resourceid.h"

//------------------------------------------------------------------------------
namespace RenderUtil
{
class NodeLookupUtil
{
public:
    /// find state node instance
    static Ptr<Models::StateNodeInstance> LookupStateNodeInstance(const Ptr<Graphics::ModelEntity>& entity,
                                                                  const Util::String& nodeName, 
                                                                  bool checkResourceState = true);

    /// find state node instance
    static Ptr<Models::AnimatorNodeInstance> LookupAnimatorNodeInstance(const Ptr<Graphics::ModelEntity>& entity,
                                                                        const Util::String& nodeName, 
                                                                        bool checkResourceState = true);
private:
	// gets the names of modelNode and all its children's children children children	
	static Util::Array<Util::String> RecursiveGetNodeNames(const Ptr<Models::ModelNode>& modelNode /*rootNode*/, const Util::FourCC & fourcc);
	static Util::String currentNodeLayer;
};

} // namespace RenderUtil
//------------------------------------------------------------------------------

