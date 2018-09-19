//------------------------------------------------------------------------------
// modelcontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "modelcontext.h"
#include "resources/resourcemanager.h"
#include "nodes/modelnode.h"
#include "streammodelpool.h"
#include "graphics/graphicsserver.h"

using namespace Graphics;
using namespace Resources;
namespace Models
{

ImplementContext(ModelContext);
ModelContext::ModelContextAllocator ModelContext::modelContextAllocator;
//------------------------------------------------------------------------------
/**
*/
ModelContext::ModelContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ModelContext::~ModelContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::Create()
{
	__bundle.OnBeforeFrame = ModelContext::OnBeforeFrame;
	__bundle.OnWaitForWork = nullptr;
	__bundle.OnBeforeView = ModelContext::OnBeforeView;
	__bundle.OnAfterView = ModelContext::OnAfterView;
	__bundle.OnAfterFrame = ModelContext::OnAfterFrame;
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle);

	CreateContext();
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::Setup(const Graphics::GraphicsEntityId id, const Resources::ResourceName& name, const Util::StringAtom& tag)
{
	const ContextEntityId cid = GetContextId(id);
	ModelId& rid = modelContextAllocator.Get<0>(cid.id);
	modelContextAllocator.Get<1>(cid.id) = ModelInstanceId::Invalid();

	ModelCreateInfo info;
	info.resource = name;
	info.tag = tag;
	info.async = true;
	info.failCallback = nullptr;
	info.successCallback = [cid](Resources::ResourceId id)
	{
		ModelInstanceId& mdl = modelContextAllocator.Get<1>(cid.id);
		mdl = Models::CreateModelInstance(id);
		const Math::matrix44& pending = modelContextAllocator.Get<2>(cid.id);
		Models::modelPool->modelInstanceAllocator.Get<2>(mdl.instance) = pending;
	};

	rid = Models::CreateModel(info);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::ChangeModel(const Graphics::GraphicsEntityId id, const Resources::ResourceName& name, const Util::StringAtom& tag)
{
	const ContextEntityId cid = GetContextId(id);

	// clean up old stuff, but don't deallocate entity
	ModelId& rid = modelContextAllocator.Get<0>(cid.id);
	ModelInstanceId& mdl = modelContextAllocator.Get<1>(cid.id);
	mdl = ModelInstanceId::Invalid();

	if (rid != ModelId::Invalid()) // decrement model resource
		Models::DestroyModel(rid);
	if (mdl != ModelInstanceId::Invalid()) // actually deallocate current instance
		Models::DestroyModelInstance(mdl);

	ModelCreateInfo info;
	info.resource = name;
	info.tag = tag;
	info.async = true;
	info.failCallback = nullptr;
	info.successCallback = [&mdl, rid, cid](Resources::ResourceId id)
	{
		mdl = Models::CreateModelInstance(id);
		const Math::matrix44& pending = modelContextAllocator.Get<2>(cid.id);
		Models::modelPool->modelInstanceAllocator.Get<2>(mdl.instance) = pending;
	};

	rid = Models::CreateModel(info);
}

//------------------------------------------------------------------------------
/**
*/
const Models::ModelInstanceId
ModelContext::GetModel(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return modelContextAllocator.Get<1>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::SetTransform(const Graphics::GraphicsEntityId id, const Math::matrix44& transform)
{
	const ContextEntityId cid = GetContextId(id);
	ModelInstanceId& inst = modelContextAllocator.Get<1>(cid.id);
	Math::matrix44& pending = modelContextAllocator.Get<2>(cid.id);
	bool& hasPending = modelContextAllocator.Get<3>(cid.id);
	pending = transform;
	hasPending = true;
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
ModelContext::GetTransform(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	ModelInstanceId& inst = modelContextAllocator.Get<1>(cid.id);
	return modelContextAllocator.Get<2>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::ModelNode::Instance*>& 
ModelContext::GetModelNodeInstances(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	ModelInstanceId& inst = modelContextAllocator.Get<1>(cid.id);
	return Models::modelPool->modelInstanceAllocator.Get<0>(inst.instance);
}

//------------------------------------------------------------------------------
/**
	Go through all models and apply their transforms
*/
void
ModelContext::OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime)
{
	const Util::Array<ModelInstanceId>& instances = modelContextAllocator.GetArray<1>();
	const Util::Array<Math::matrix44>& transforms = Models::modelPool->modelInstanceAllocator.GetArray<1>();
	const Util::Array<Math::bbox>& modelBoxes = Models::modelPool->modelAllocator.GetArray<0>();
	Util::Array<Math::bbox>& instanceBoxes = Models::modelPool->modelInstanceAllocator.GetArray<2>();
	Util::Array<Math::matrix44>& pending = modelContextAllocator.GetArray<2>();
	Util::Array<bool>& hasPending = modelContextAllocator.GetArray<3>();
	
	SizeT i;
	for (i = 0; i < instances.Size(); i++)
	{
		const ModelInstanceId& instance = instances[i];
		if (instance == ModelInstanceId::Invalid()) continue; // hmm, bad, should reorder and keep a 'last valid index'

		// get reference so we can include it in the pending
		Math::matrix44 transform = transforms[instance.instance];

		// if we have a pending transform, apply it and transform bounding box
		if (hasPending[i])
		{
			transform = Math::matrix44::multiply(transform, pending[i]);
			hasPending[i] = false;

			// transform the box
			instanceBoxes[instance.instance] = modelBoxes[instance.model];
			instanceBoxes[instance.instance].affine_transform(transform);

			// update the actual transform
			transforms[instance.instance] = transform;

			Util::Array<Models::ModelNode::Instance*>& nodes = Models::modelPool->modelInstanceAllocator.Get<0>(instance.instance);

			// nodes are allocated breadth first, so just going through the list will guarantee the hierarchy is traversed in proper order
			SizeT j;
			for (j = 0; j < nodes.Size(); j++)
			{
				Models::ModelNode::Instance* node = nodes[j];
				Math::matrix44& parentTransform = transform;
				if (node->parent->type > NodeHasTransform && node->parent != nullptr)
					parentTransform = static_cast<const TransformNode::Instance*>(node->parent)->modelTransform;

				if (node->type > NodeHasTransform)
				{
					TransformNode::Instance* tnode = static_cast<TransformNode::Instance*>(node);
					tnode->modelTransform = Math::matrix44::multiply(tnode->transform.getmatrix(), parentTransform);
					parentTransform = tnode->modelTransform;
				}
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::OnWaitForWork(const IndexT frameIndex, const Timing::Time frameTime)
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