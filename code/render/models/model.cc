//------------------------------------------------------------------------------
//  model.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "models/model.h"
#include "models/modelinstance.h"
#include "models/modelserver.h"
#include "models/visresolver.h"
#include "models/nodes/transformnodeinstance.h"
#include "resources/resourcemanager.h"

namespace Models
{
__ImplementClass(Models::Model, 'MODL', Resources::Resource);

using namespace Util;
using namespace Resources;
using namespace Math;

//------------------------------------------------------------------------------
/**
*/
Model::Model() :
    inLoadResources(false),
    resourcesLoaded(false)
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
Model::~Model()
{
    // empty
}

//------------------------------------------------------------------------------
/**
*/
void
Model::Unload()
{    
    // discard model nodes
    IndexT i;
    for (i = 0; i < this->nodes.Size(); i++)
    {
        this->nodes[i]->OnRemoveFromModel();
    }
    this->nodes.Clear();
    this->visibleModelNodes.Reset();

    // discard instances
    for (i = 0; i < this->instances.Size(); i++)
    {
        this->instances[i]->Discard();
    }
    this->instances.Clear();

    // call parent class
    Resource::Unload();
}

//------------------------------------------------------------------------------
/**
*/
void
Model::Reload()
{
	// be careful with the instances however, they all need to be reloaded, so we don't clear the array of instances
	IndexT i;
	for (i = 0; i < this->instances.Size(); i++)
	{
		this->instances[i]->Discard();
	}

	// unload node resources
	this->UnloadResources();

	// unload all nodes manually
	for (i = 0; i < this->nodes.Size(); i++)
	{
		this->nodes[i]->OnRemoveFromModel();
	}
	this->nodes.Clear();
	this->visibleModelNodes.Reset();

	// run base class unload
	Resource::Unload();

	// reset loader
	this->loader->Reset();

	// get asynchronous flag
	bool asyncEnabled = this->asyncEnabled;

	// disable asynchronous here
	this->SetAsyncEnabled(false);

	// then simply load it again
	this->Load();

	// tag resources to not be loaded
	this->resourcesLoaded = false;

	// tag resource manager to wait for pending resources
	Resources::ResourceManager::Instance()->WaitForPendingResources(0);

	// now check pending resources
	this->CheckPendingResources();

	// reset to previous state
	this->SetAsyncEnabled(asyncEnabled);

	// seeing as we want to reset our previous state, we can just setup our instances again!
	for (i = 0; i < this->instances.Size(); i++)
	{
		this->instances[i]->Setup(this, this->GetRootNode());
	}
}

//------------------------------------------------------------------------------
/**
    This method asks all model nodes to load their resources. Note that 
    actual resource loading may be asynchronous and placeholder resources
    may be in place right after this method returns.
*/
void
Model::LoadResources()
{
    n_assert(!this->inLoadResources);
    IndexT i;
    for (i = 0; i < this->nodes.Size(); i++)
    {
        this->nodes[i]->LoadResources(!this->asyncEnabled);
    }
	this->inLoadResources = true;
}

//------------------------------------------------------------------------------
/**
    This method asks all model nodes to unload their resources.
*/
void
Model::UnloadResources()
{
    n_assert(this->inLoadResources);
    IndexT i;
    for (i = 0; i < this->nodes.Size(); i++)
    {
        this->nodes[i]->UnloadResources();
    }
    this->inLoadResources = false;
}

//------------------------------------------------------------------------------
/**
*/
void
Model::ReloadResources()
{
	n_assert(!this->inLoadResources);
	IndexT i;
	for (i = 0; i < this->nodes.Size(); i++)
	{
		this->nodes[i]->ReloadResources();
	}
	this->inLoadResources = true;
}


//------------------------------------------------------------------------------
/**
    This checks whether all resource have been loaded, if yes the method
    OnResourcesLoaded() will be called once. If some resources are not
    loaded yet, the method will return false.
*/
bool
Model::CheckPendingResources()
{
    n_assert(this->inLoadResources);
    Resource::State result = Resource::Initial;
    IndexT i;
    for (i = 0; i < this->nodes.Size(); i++)
    {
        Resource::State state = this->nodes[i]->GetResourceState();
        if (state > result)
        {
            result = state;
        }
    }
    if (Resource::Loaded == result)
    {
        if (!this->resourcesLoaded)
        {
            // all resources have been loaded
            this->OnResourcesLoaded();
        }
        return true;
    }
    else
    {
        // not all resources have been loaded yet
        return false;
    }
}

//------------------------------------------------------------------------------
/**
    This method is called once when all pending asynchronous resources have 
    been loaded (the Model is ready for rendering), this is when
    Model::GetResourceState() returns Loaded for the first time.
*/
void
Model::OnResourcesLoaded()
{
    n_assert(!this->resourcesLoaded);
    this->resourcesLoaded = true;
    IndexT i;
    for (i = 0; i < this->nodes.Size(); i++)
    {
        this->nodes[i]->OnResourcesLoaded();
    }
}

//------------------------------------------------------------------------------
/**
*/
void
Model::AttachNode(const Ptr<ModelNode>& node)
{
    n_assert(node->GetName().IsValid());
    this->nodes.Append(node);
    node->OnAttachToModel(this);
}

//------------------------------------------------------------------------------
/**
*/
void
Model::RemoveNode(const Ptr<ModelNode>& node)
{
    IndexT nodeIndex = this->nodes.FindIndex(node);
    n_assert(InvalidIndex != nodeIndex);
	// FIXME, should remove from parents children list as well, why wasnt this done before?
	if(node->GetParent())
	{
		node->GetParent()->RemoveChild(node);
	}
    node->OnRemoveFromModel();
    this->nodes.EraseIndex(nodeIndex);
}

//------------------------------------------------------------------------------
/**
*/
void
Model::AddVisibleModelNode(IndexT frameIndex, const Materials::SurfaceName::Code& code, const Ptr<ModelNode>& modelNode)
{
    this->visibleModelNodes.Add(frameIndex, code, modelNode);
    if (!this->visibleModelNodes.IsResolved(code))
    {
        this->visibleModelNodes.SetResolved(code, true);
        VisResolver::Instance()->AddVisibleModel(frameIndex, code, this);
    }
}

//------------------------------------------------------------------------------
/**
*/
Ptr<ModelInstance>
Model::CreateInstance()
{
    // create a new ModelInstance from the root node
    Ptr<ModelInstance> modelInstance = ModelInstance::Create();
    modelInstance->Setup(this, this->GetRootNode());
    this->instances.Append(modelInstance);
    return modelInstance;
}

//------------------------------------------------------------------------------
/**
*/
Ptr<ModelInstance>
Model::CreatePartialInstance(const StringAtom& rootNodePath, const matrix44& rootNodeOffsetMatrix)
{
    // lookup the root node and create a new ModelInstance
    Ptr<ModelInstance> modelInstance = ModelInstance::Create();
    Ptr<ModelNode> treeRootNode = this->LookupNode(rootNodePath.AsString());
    n_assert(treeRootNode.isvalid());
    modelInstance->Setup(this, treeRootNode);
    this->instances.Append(modelInstance);

    // fixup the root node instance matrix with the offset matrix
    const Ptr<TransformNodeInstance>& rootNodeInst = modelInstance->GetRootNodeInstance().downcast<TransformNodeInstance>();
    rootNodeInst->SetOffsetMatrix(rootNodeOffsetMatrix);

    return modelInstance;
}

//------------------------------------------------------------------------------
/**
*/
void
Model::DiscardInstance(Ptr<ModelInstance> modelInst)
{
    IndexT modelInstIndex = this->instances.FindIndex(modelInst);
    n_assert(modelInstIndex != InvalidIndex);
    this->instances.EraseIndex(modelInstIndex);
    modelInst->Discard();
}

//------------------------------------------------------------------------------
/**
    This method will update the Model's bounding box to include the 
    bounding boxes of all ModelNodes owned by the Model object.
*/
void
Model::UpdateBoundingBox()
{
    n_assert(this->nodes.Size() > 0);
    this->boundingBox.begin_extend();
    IndexT i;
    for (i = 0; i < this->nodes.Size(); i++)
    {
        this->boundingBox.extend(this->nodes[i]->GetBoundingBox());
    }
	this->boundingBox.end_extend();
}

//------------------------------------------------------------------------------
/**
    Careful, this method is SLOW!
*/
Ptr<ModelNode>
Model::LookupNode(const String& path) const
{
    Ptr<ModelNode> curPtr;
    if (this->nodes.Size() > 0)
    {
        Array<String> tokens = path.Tokenize("/");
        n_assert(tokens.Size() > 0);
        curPtr = this->GetRootNode();
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
                    return Ptr<ModelNode>();
                }
            }
        }
        else
        {
            // return an invalid ptr
            return Ptr<ModelNode>();
        }
    }
    return curPtr;
}

} // namespace Models
