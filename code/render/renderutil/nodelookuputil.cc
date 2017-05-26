//------------------------------------------------------------------------------
//  nodelookuputil.cc
//  (C) 2009 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "renderutil/nodelookuputil.h"
#include "graphics/modelentity.h"
#include "graphics/graphicsserver.h"
#include "graphics/view.h"
#include "graphics/graphicsentity.h"
#include "models/modelinstance.h"
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
Ptr<StateNodeInstance>
NodeLookupUtil::LookupStateNodeInstance(const Ptr<ModelEntity>& entity, 
                                        const Util::String& nodeName, 
                                        bool checkResourceState /* = true */)
{
    n_assert(entity->GetType() == Graphics::GraphicsEntityType::Model);

    Ptr<ModelEntity> modelEntity = entity.cast<ModelEntity>();
    // is model entity deleted, and msg out-of-date, return handled = true to remove msg from list
    if (!modelEntity->IsActive())
    {
        return Ptr<StateNodeInstance>(0);
    }

    // check resource state if set to
    if (checkResourceState && modelEntity->GetModelResourceState() != Resources::Resource::Loaded)
    {
        return Ptr<StateNodeInstance>(0);
    }

    // check model instance
    const Ptr<ModelInstance>& modelInst =  modelEntity->GetModelInstance();    
    if (!modelInst.isvalid())
    {
        return Ptr<StateNodeInstance>(0);
    }

    // lookup, cast and return
    const Ptr<ModelNodeInstance>& nodeInst = modelInst->LookupNodeInstance(nodeName);
    n_assert(nodeInst->IsA(StateNodeInstance::RTTI));
    const Ptr<StateNodeInstance>& stateNodeInst = nodeInst.cast<StateNodeInstance>();
    return stateNodeInst;
}

//------------------------------------------------------------------------------
/**
    FIXME: THESE METHODS ARE SLOW AS HELL!!!

    Utility function which searches a specified animatornodeinstance in a 
    graphicsentity.

    returns 0 if 
    - resource is not loaded and resourcecheck flag is enabled
    - node was not found
    - modelentity is not active
    - model instances is not valid
*/
Ptr<AnimatorNodeInstance>
NodeLookupUtil::LookupAnimatorNodeInstance(const Ptr<ModelEntity>& entity, 
                                           const Util::String& nodeName, 
                                           bool checkResourceState)
{
    n_assert(entity->GetType() == Graphics::GraphicsEntityType::Model);

    Ptr<ModelEntity> modelEntity = entity.cast<ModelEntity>();
    // is modelentity deleted, and msg out-of-date, return handled = true to remove msg from list
    if (!modelEntity->IsActive())
    {
        return Ptr<AnimatorNodeInstance>(0);
    }

    // check resource state if set to
    if (checkResourceState && modelEntity->GetModelResourceState() != Resources::Resource::Loaded)
    {
        return Ptr<AnimatorNodeInstance>(0);
    }

    // check model instance
    const Ptr<ModelInstance>& modelInst =  modelEntity->GetModelInstance();    
    if (!modelInst.isvalid())
    {
        return Ptr<AnimatorNodeInstance>(0);
    }

    // lookup, cast and return
    const Ptr<ModelNodeInstance>& nodeInst = modelInst->LookupNodeInstance(nodeName);
    n_assert(nodeInst->IsA(AnimatorNodeInstance::RTTI));
    const Ptr<AnimatorNodeInstance>& animatorNodeInst = nodeInst.cast<AnimatorNodeInstance>();
    return animatorNodeInst;
}



//------------------------------------------------------------------------------
/**
*/
Util::Array<Util::String> 
NodeLookupUtil::RecursiveGetNodeNames( const Ptr<ModelNode>& modelNode /*rootNode*/,const Util::FourCC & fourcc )
{
	Util::Array<Util::String> _resultList;
	Util::Array<Ptr<ModelNode>> modelNodeList = modelNode->GetChildren();
	Util::String nodeName = modelNode->GetName().AsString();

	IndexT index;
	currentNodeLayer.Append(nodeName + "/");
	for(index = 0; index < modelNodeList.Size(); index++)
	{
		_resultList.AppendArray(RecursiveGetNodeNames(modelNodeList[index],fourcc));
		
	}
	Util::Array<Util::String> _layerList = currentNodeLayer.Tokenize("/");
	currentNodeLayer = "";
	for(int i =0; i<_layerList.Size()-1; i++ )
	{
		currentNodeLayer += _layerList[i] + "/";
	}
	if(fourcc.IsValid())
	{
		if(modelNode->IsA(fourcc))
		{
			_resultList.Append(currentNodeLayer + nodeName);
		}

	}else{
		_resultList.Append(currentNodeLayer + nodeName);
	}

	return _resultList;
}
} // namespace RenderUtil
