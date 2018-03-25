//------------------------------------------------------------------------------
// modelcontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "modelcontext.h"
#include "resources/resourcemanager.h"
#include "models/model.h"
#include "nodes/modelnode.h"
#include "streammodelpool.h"

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
ModelContext::Setup(const Graphics::GraphicsEntityId id, const Resources::ResourceName& name, const Util::StringAtom& tag)
{
	const ContextEntityId cid = this->entitySliceMap[id.id];
	ModelId& rid = this->modelContextAllocator.Get<0>(cid.id);
	ModelInstanceId& mdl = this->modelContextAllocator.Get<1>(cid.id);
	mdl = ModelInstanceId::Invalid();

	ModelCreateInfo info;
	info.resource = name;
	info.tag = tag;
	info.async = true;
	info.failCallback = nullptr;
	info.successCallback = [&mdl, rid, this](Resources::ResourceId id)
	{
		mdl = Models::CreateModelInstance(id);
	};

	rid = Models::CreateModel(info);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::ChangeModel(const Graphics::GraphicsEntityId id, const Resources::ResourceName& name, const Util::StringAtom& tag)
{
	const ContextEntityId cid = this->entitySliceMap[id.id];

	// clean up old stuff, but don't deallocate entity
	ModelId& rid = this->modelContextAllocator.Get<0>(cid.id);
	ModelInstanceId& mdl = this->modelContextAllocator.Get<1>(cid.id);

	if (rid != ModelId::Invalid())
		Models::DestroyModel(rid);
	if (mdl != ModelInstanceId::Invalid()) 
		Models::DestroyModelInstance(mdl);

	ModelCreateInfo info;
	info.resource = name;
	info.tag = tag;
	info.async = true;
	info.failCallback = nullptr;
	info.successCallback = [&mdl, rid, this](Resources::ResourceId id)
	{
		mdl = Models::CreateModelInstance(id);
	};

	rid = Models::CreateModel(info);
}

//------------------------------------------------------------------------------
/**
*/
const Models::ModelInstanceId
ModelContext::GetModel(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = this->entitySliceMap[id.id];
	return this->modelContextAllocator.Get<1>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::SetTransform(const Graphics::GraphicsEntityId id, const Math::matrix44 transform)
{
	const ContextEntityId cid = this->entitySliceMap[id.id];
	Models::modelPool->modelInstanceAllocator.Get<3>(cid.id) = transform;
}

//------------------------------------------------------------------------------
/**
	Go through all models and apply their transforms
*/
void
ModelContext::OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime)
{
	const Util::Array<ModelInstanceId>& instances = this->modelContextAllocator.GetArray<1>();
	const Util::Array<Math::matrix44>& transforms = Models::modelPool->modelInstanceAllocator.GetArray<2>();
	const Util::Array<Math::bbox>& modelBoxes = Models::modelPool->modelAllocator.GetArray<0>();
	Util::Array<Math::bbox>& instanceBoxes = Models::modelPool->modelInstanceAllocator.GetArray<3>();

	SizeT i;
	for (i = 0; i < instances.Size(); i++)
	{
		const ModelInstanceId& instance = instances[i];
		instanceBoxes[instance.instance] = modelBoxes[instance.model];
		instanceBoxes[instance.instance].transform(transforms[instance.instance]);

		Util::Array<Models::ModelNode::Instance*>& nodes = Models::modelPool->modelInstanceAllocator.Get<1>(instance.instance);

		// nodes are allocated breadth first, so just going through the list will guarantee the hierarchy is intact
		SizeT j;
		for (j = 0; j < nodes.Size(); j++)
		{
			Math::matrix44 parentTransform = transforms[instance.instance];
			if (nodes[j]->parent != nullptr && nodes[j]->type > NodeHasTransform)
				parentTransform = static_cast<TransformNode::Instance*>(nodes[j]->parent)->modelTransform;

			if (nodes[j]->type > NodeHasTransform)
			{
				TransformNode::Instance* tnode = static_cast<TransformNode::Instance*>(nodes[j]);
				tnode->modelTransform = Math::matrix44::multiply(tnode->transform.getmatrix(), transforms[instance.instance]);
				parentTransform = tnode->modelTransform;
			}

			// reset bounding box
			nodes[j]->boundingBox = nodes[j]->node->boundingBox;
			nodes[j]->boundingBox.transform(parentTransform);
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::OnVisibilityReady(const IndexT frameIndex, const Timing::Time frameTime)
{
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::OnBeforeView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime)
{
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::OnAfterView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime)
{
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::OnAfterFrame(const IndexT frameIndex, const Timing::Time frameTime)
{
}

} // namespace Models