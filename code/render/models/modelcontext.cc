//------------------------------------------------------------------------------
// modelcontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "modelcontext.h"
#include "resources/resourcemanager.h"
#include "models/model.h"
#include "nodes/modelnode.h"

using namespace Graphics;
using namespace Resources;
namespace Models
{

__ImplementClass(Models::ModelContext, 'MOCO', Graphics::GraphicsContext);
__ImplementSingleton(Models::ModelContext);
//------------------------------------------------------------------------------
/**
*/
ModelContext::ModelContext()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
ModelContext::~ModelContext()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::Setup(const Graphics::GraphicsEntityId id, const Resources::ResourceName& name)
{
	const Ids::Id32 cid = this->entitySliceMap[id.id];
	Resources::ResourceId& rid = this->modelContextAllocator.Get<0>(cid);
	ModelInstanceId& mdl = this->modelContextAllocator.Get<1>(cid);

	// create resource
	rid = Resources::CreateResource(name, ""_atm, [&mdl, rid, this](Resources::ResourceId id)
	{
		// create instance of resource once done
		mdl = this->CreateModelInstance(rid);
	});
}

//------------------------------------------------------------------------------
/**
*/
const Models::ModelInstanceId
ModelContext::GetModel(const Graphics::GraphicsEntityId id)
{
	const Ids::Id32 cid = this->entitySliceMap[id.id];
	return this->modelContextAllocator.Get<1>(cid);
}

//------------------------------------------------------------------------------
/**
	IMPLEMENT ME!
*/
Models::ModelInstanceId
ModelContext::CreateModelInstance(const Resources::ResourceId id)
{
	Models::ModelInstanceId id;
	Ids::Id32 miid = this->modelInstanceAllocator.AllocObject();

	// this should be a model
	Models::ModelId mid;
	mid.id = id.allocId;

	// get nodes
	const Util::Dictionary<Util::StringAtom, ModelNodeId>& nodes = ModelGetNodes(mid);

	this->modelInstanceAllocator.Get<0>(miid) = mid;
	this->modelInstanceAllocator.Get<1>(miid) = Math::matrix44::identity();
	this->modelInstanceAllocator.Get<2>(miid)

	/*
	Ptr<Model> model = Resources::GetResource<Model>(id);
	Ids::Id32 instanceId;
	if (!this->modelInstancePool.Allocate(instanceId))
	{
		// append new list, this might grow the list
		this->modelInstances.Append(Util::Array<NodeInstance>());
	}

	// grab list
	Util::Array<NodeInstance>& nodes = this->modelInstances[Ids::Index(instanceId)];
	nodes.Reserve(model->nodes.Size());

	IndexT i;
	for (i = 0; i < nodes.Size(); i++)
	{
		const Ptr<ModelNode>& node = model->nodes.ValueAtIndex(i);
		const Ptr<ModelNode>& parent = node->parent;

		if (node->IsA(PrimitiveNode::RTTI))
		{

		}
	}
	*/
	return Models::ModelInstanceId();
}

} // namespace Models