//------------------------------------------------------------------------------
//  modelinstance.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "models/modelinstance.h"
#include "models/modelnodeinstance.h"
#include "graphics/modelentity.h"

namespace Models
{
__ImplementClass(Models::ModelInstance, 'MODI', Core::RefCounted);

using namespace Util;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
ModelInstance::ModelInstance() :
	visibilityResolveIndex(InvalidIndex),
	viewDistanceThisFrame(0),
	instanced(false)
{
    this->transform = matrix44::identity();
}

//------------------------------------------------------------------------------
/**
*/
ModelInstance::~ModelInstance()
{
    n_assert(!this->IsValid());
}

//------------------------------------------------------------------------------
/**
*/
void
ModelInstance::Setup(const Ptr<Model>& m, const Ptr<ModelNode>& rootNode)
{
    n_assert(!this->IsValid());
    this->model = m;

    // create node instances
    rootNode->CreateNodeInstanceHierarchy(this);	
}

//------------------------------------------------------------------------------
/**
*/
void
ModelInstance::Discard()
{
    n_assert(this->IsValid());
    this->model = 0;

    // discard node instances
    while (!this->nodeInstances.IsEmpty())
    {
        // need to keep a reference to the node when calling Discard()!
        Ptr<ModelNodeInstance> curNodeInstance = this->nodeInstances.Back();
        curNodeInstance->Discard();
    }	
}

//------------------------------------------------------------------------------
/**
*/
void
ModelInstance::SetModelEntity(const Ptr<Graphics::ModelEntity>& mdlEntity)
{
    this->modelEntity = mdlEntity;
}

//------------------------------------------------------------------------------
/**
*/
const Ptr<Graphics::ModelEntity>&
ModelInstance::GetModelEntity() const
{
    return this->modelEntity;
}

//------------------------------------------------------------------------------
/**
    Get a pointer to the root node instance. This is always
    the first node instance.
*/
const Ptr<ModelNodeInstance>&
ModelInstance::GetRootNodeInstance() const
{
    n_assert(!this->nodeInstances.IsEmpty());
    n_assert(!this->nodeInstances[0]->HasParent());
    return this->nodeInstances[0];
}

//------------------------------------------------------------------------------
/**
    Attach a node instance which has already been setup.
*/
void
ModelInstance::AttachNodeInstance(const Ptr<ModelNodeInstance>& nodeInst)
{
    n_assert(InvalidIndex == this->nodeInstances.FindIndex(nodeInst));
    this->nodeInstances.Append(nodeInst);
}

//------------------------------------------------------------------------------
/**
    Remove a node instance, do not discard it!
*/
void
ModelInstance::RemoveNodeInstance(const Ptr<ModelNodeInstance>& nodeInst)
{
    IndexT index = this->nodeInstances.FindIndex(nodeInst);
    n_assert(InvalidIndex != index);
    this->nodeInstances.EraseIndex(index);
}

//------------------------------------------------------------------------------
/**
    This method is called once per frame on ModelInstances which have
    been detected as visible during the culling phase (so, relatively early).
*/
void
ModelInstance::OnNotifyCullingVisible(IndexT frameIndex, Timing::Time time)
{
    IndexT i;
    for (i = 0; i < this->nodeInstances.Size(); i++)
    {
        const Ptr<ModelNodeInstance>& nodeInst = this->nodeInstances[i];
        if (nodeInst->IsVisible())
        {
            #if NEBULA3_ENABLE_PROFILING
            nodeInst->ResetDebugTimer();
            #endif
            nodeInst->OnNotifyCullingVisible(frameIndex, time);
        }
    }
}

//------------------------------------------------------------------------------
/**
    This method is called once per frame on visible ModelInstances right
    before rendering.
*/
void
ModelInstance::OnRenderBefore(IndexT frameIndex, Timing::Time time)
{
    IndexT i;
    for (i = 0; i < this->nodeInstances.Size(); i++)
    {
        const Ptr<ModelNodeInstance>& nodeInst = this->nodeInstances[i];
        if (nodeInst->IsVisible())
        {
            nodeInst->OnRenderBefore(frameIndex, time);
            nodeInst->GetModelNode()->ResetScreenSpaceStats();
        }
    }
}

//------------------------------------------------------------------------------
/**
    This method is called by the Graphics subsystem when we are currently
    visible. Once all visible model instances are notified, the Graphics
    subsystem can get a render-order-optimized list of all visible
    model-node-instances through the ModelServer.
*/
void 
ModelInstance::OnVisibilityResolve(IndexT frameIndex, IndexT resolveIndex, const Math::matrix44& cameraTransform, bool updateLod)
{
	if (updateLod)
	{
		// compute our distance to the camera
		float4 viewVector = cameraTransform.get_position() - this->transform.get_position();
		this->viewDistanceThisFrame = viewVector.length();
		this->visibilityResolveIndex = frameIndex;
	}

    // all we need to do is to distribute the method to all our model node instances
    IndexT i;
    SizeT num = this->nodeInstances.Size();
    for (i = 0; i < num; i++)
    {
        // don't notify invisible node instances
        if (this->nodeInstances[i]->IsVisible())
        {
			this->nodeInstances[i]->OnVisibilityResolve(frameIndex, resolveIndex, this->viewDistanceThisFrame);
        }
    }
}

//------------------------------------------------------------------------------
/**
    This updates the render stats of the model.
    This may only be called by the players camera as other camera transforms (e.g. for light and shadow)
    shall not influence the LOD of a model.
*/
void
ModelInstance::UpdateRenderStats(const Math::matrix44& cameraTransform)
{
    // c. 135 is 1:1 --> everything lower needs max. mip level, each higher multiple of 135 can be rendered
    // using appropriate lower mip level
	float resourceStreamingLevelOfDetail = ((cameraTransform.get_position() - this->transform.get_position()).length() - 38.5f) / 38.5f;
    if (resourceStreamingLevelOfDetail < 0.0f)
    {
        resourceStreamingLevelOfDetail = 0.0f;
    }


    // all we need to do is to distribute the method to all our model node instances
    IndexT i;
    SizeT num = this->nodeInstances.Size();
    for (i = 0; i < num; i++)
    {
        // don't notify invisible node instances
        if (this->nodeInstances[i]->IsVisible())
        {
            this->nodeInstances[i]->GetModelNode()->SetResourceStreamingLevelOfDetail(resourceStreamingLevelOfDetail);
        }
    }
}

//------------------------------------------------------------------------------
/**
    This method is called from the RenderDebug of Graphics::ModelEntity.
*/
void 
ModelInstance::RenderDebug()
{
    IndexT i;
    SizeT num = this->nodeInstances.Size();
    for (i = 0; i < num; i++)
    {
        this->nodeInstances[i]->RenderDebug();
    }
}

//------------------------------------------------------------------------------
/**
    Careful, this method is SLOW!
*/
Ptr<ModelNodeInstance>
ModelInstance::LookupNodeInstance(const String& path) const
{
    n_assert(path.Length() > 0);
    n_assert(this->nodeInstances.Size() > 0);

    // go through all root node instances
    IndexT i;
    for (i = 0; i < this->nodeInstances.Size(); i++)
    {
		Ptr<ModelNodeInstance> node = this->LookupNodeInstanceHelper(path, nodeInstances[i]);
        if (node.isvalid()) return node;
    }

    return Ptr<ModelNodeInstance>();
    /*
    Ptr<ModelNodeInstance> curPtr;
    if (this->nodeInstances.Size() > 0)
    {
        Array<String> tokens = path.AsString().Tokenize("/");
        n_assert(tokens.Size() > 0);
        curPtr = this->GetRootNodeInstance();
        if (tokens[0] == curPtr->GetName().Value())
        {
            IndexT i;
            for (i = 1; i < tokens.Size(); i++)
            {
                StringAtom curToken = tokens[i];
                if (curPtr->HasChild(curToken))
                {
                    curPtr = curPtr->LookupChild(curToken);
                }
                else
                {
                    // return an invalid ptr
                    return Ptr<ModelNodeInstance>();
                }
            }
        }
    }
    return curPtr;
    */
}

//------------------------------------------------------------------------------
/**
*/
Ptr<ModelNodeInstance> 
ModelInstance::LookupNodeInstanceHelper( const Util::String& path, const Ptr<ModelNodeInstance>& node ) const
{
    IndexT location = path.FindCharIndex('/');
    String currentToken;
    String rest;
    if (InvalidIndex != location)
    {
        currentToken = path.ExtractRange(0, location);
        rest = path.ExtractToEnd(location + 1);
    }
    else
    {
        currentToken = path;
        n_assert2(currentToken != "*", "Search criteria must end with a fixed name");
    }

	if (node->GetName() == currentToken || currentToken == "*")
	{
		if (rest.IsEmpty())
		{
			return node;
		}
		else
		{
			const Util::Array<Ptr<ModelNodeInstance>>& children = node->GetChildren();
			IndexT i;
			for (i = 0; i < children.Size(); i++)
			{
				Ptr<ModelNodeInstance> locatedNode = this->LookupNodeInstanceHelper(rest, children[i]);
				if (locatedNode.isvalid()) return locatedNode;
			}
		}			
	}

    // return null pointer if never found
    return Ptr<ModelNodeInstance>();
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelInstance::OnShow(Timing::Time time)
{
    IndexT i;
    SizeT num = this->nodeInstances.Size();
    for (i = 0; i < num; i++)
    {
        //this->nodeInstances[i]->OnShow(time); //SIMON fix?
		this->nodeInstances[i]->SetVisible(true,time);
    }
}

//------------------------------------------------------------------------------
/**
*/
void 
ModelInstance::OnHide(Timing::Time time)
{
    IndexT i;
    SizeT num = this->nodeInstances.Size();
    for (i = 0; i < num; i++)
    {
        //this->nodeInstances[i]->OnHide(time); //SIMON fix?
		this->nodeInstances[i]->SetVisible(false,time);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ModelInstance::SetTransform(const Math::matrix44& m)
{
	this->transform = m;	
}


} // namespace Models
