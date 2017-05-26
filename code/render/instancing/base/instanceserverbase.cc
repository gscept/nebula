//------------------------------------------------------------------------------
//  instanceserverbase.cc
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "instanceserverbase.h"
#include "instancing/instancerenderer.h"
#include "graphics/batchgroup.h"

using namespace Models;
namespace Base
{
__ImplementSingleton(Base::InstanceServerBase);
__ImplementClass(Base::InstanceServerBase, 'INSB', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
InstanceServerBase::InstanceServerBase() :
	renderer(0),
	modelNode(0), 
    shader(0),
    pass(-1),
	isBeginInstancing(false),
	isOpen(false),
	multiplier(1)	
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
InstanceServerBase::~InstanceServerBase()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
bool
InstanceServerBase::Open()
{
	n_assert(!this->IsOpen());
	this->isOpen = true;
	return true;
}

//------------------------------------------------------------------------------
/**
*/
void
InstanceServerBase::Close()
{
	n_assert(this->IsOpen());
	this->modelNode = 0;
	this->instancesByCode.Clear();
	this->isOpen = false;
}

//------------------------------------------------------------------------------
/**
	Prepare to render model.
*/
void 
InstanceServerBase::BeginInstancing(const Ptr<ModelNode>& modelNode, const SizeT multiplier, const Ptr<CoreGraphics::Shader>& shader, const IndexT& pass)
{
	n_assert(this->IsOpen());
	n_assert(multiplier > 0);
	n_assert(!this->isBeginInstancing);
	this->multiplier = multiplier;
	this->modelNode = modelNode;
    this->shader = shader;
	this->pass = pass;
	this->isBeginInstancing = true;
}

//------------------------------------------------------------------------------
/**
	Add model instances to list.
*/
void 
InstanceServerBase::AddInstance( const IndexT& instanceCode, const Ptr<ModelNodeInstance>& nodeInstance )
{
	n_assert(this->IsOpen());
	n_assert(this->modelNode.isvalid());
	n_assert(nodeInstance->GetModelNode() == this->modelNode);
	n_assert(this->isBeginInstancing);

	if (!this->instancesByCode.Contains(instanceCode))
	{
		this->instancesByCode.Add(instanceCode, Util::Array<Ptr<ModelNodeInstance> >());
		this->instancesByCode[instanceCode].Append(nodeInstance);
	}
	else
	{
		this->instancesByCode[instanceCode].Append(nodeInstance);
	}
}

//------------------------------------------------------------------------------
/**
	Performs actual instanced rendering, override this in a subclass.
*/
void 
InstanceServerBase::Render(IndexT frameIndex)
{
	n_error("InstanceServerBase::Render() called!");
}

//------------------------------------------------------------------------------
/**
	End the instancing.
*/
void 
InstanceServerBase::EndInstancing()
{
	n_assert(this->IsOpen());
	n_assert(this->isBeginInstancing);

	IndexT i;
	for (i = 0; i < this->instancesByCode.Size(); i++)
	{
		this->instancesByCode.ValueAtIndex(i).Clear();
	}
	this->instancesByCode.Clear();
	this->modelNode = 0;
	this->isBeginInstancing = false;
}
} // namespace Instancing
