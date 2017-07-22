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
Graphics::ContextId
ModelContext::Register(const Graphics::EntityId entity, const Resources::ResourceName& modelName)
{
	Graphics::ContextId id;
	if (!this->contextIdPool.Allocate(id))
	{
		this->modelResources.Append(Ids::InvalidId32);
		this->modelInstanceIds.Append(Ids::InvalidId32);
		this->modelInstances.Append(Util::Array<NodeInstance>());
	}

	// create resource
	Ids::Id24 index = Ids::Index(id);
	this->modelResources[index] = Resources::CreateResource(modelName, ""_atm, [this, index](Resources::ResourceId id)
	{
		// create instance of resource once done
		this->modelInstanceIds[index] = this->CreateModelInstance(index);
	});
	
	return id;
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::Unregister(const EntityId entity)
{
	this->contextIdPool.Deallocate(entity);
}

//------------------------------------------------------------------------------
/**
*/
Models::ModelInstanceId
ModelContext::CreateModelInstance(const Resources::ResourceId id)
{
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

		/* TODO: IMPLEMENT ME!
		if (node->IsA(PrimitiveNode::RTTI))
		{

		}
		*/
	}
}

} // namespace Models