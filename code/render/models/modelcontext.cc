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
	__bundle.OnVisibilityReady = ModelContext::OnVisibilityReady;
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
	ModelInstanceId& mdl = modelContextAllocator.Get<1>(cid.id);
	mdl = ModelInstanceId::Invalid();

	ModelCreateInfo info;
	info.resource = name;
	info.tag = tag;
	info.async = false;
	info.failCallback = nullptr;
	info.successCallback = [&mdl, cid](Resources::ResourceId id)
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
	info.async = false;
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
	if (inst != ModelInstanceId::Invalid())
		Models::modelPool->modelInstanceAllocator.Get<2>(inst.instance) = transform;
	else
		pending = transform;
}

//------------------------------------------------------------------------------
/**
*/
Math::matrix44
ModelContext::GetTransform(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	ModelInstanceId& inst = modelContextAllocator.Get<1>(cid.id);
	return Models::modelPool->modelInstanceAllocator.Get<2>(inst.instance);
}

//------------------------------------------------------------------------------
/**
	Go through all models and apply their transforms
*/
void
ModelContext::OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime)
{
	const Util::Array<ModelInstanceId>& instances = modelContextAllocator.GetArray<1>();
	const Util::Array<Math::matrix44>& transforms = Models::modelPool->modelInstanceAllocator.GetArray<2>();
	const Util::Array<Math::bbox>& modelBoxes = Models::modelPool->modelAllocator.GetArray<0>();
	Util::Array<Math::bbox>& instanceBoxes = Models::modelPool->modelInstanceAllocator.GetArray<3>();
	Util::Array<Math::matrix44>& pending = modelContextAllocator.GetArray<2>();
	
	SizeT i;
	for (i = 0; i < instances.Size(); i++)
	{
		const ModelInstanceId& instance = instances[i];
		if (instance == ModelInstanceId::Invalid()) continue; // hmm, bad, should reorder and keep a 'last valid index'
		instanceBoxes[instance.instance] = modelBoxes[instance.model];
		instanceBoxes[instance.instance].transform(transforms[instance.instance]);

		Util::Array<Models::ModelNode::Instance*>& nodes = Models::modelPool->modelInstanceAllocator.Get<1>(instance.instance);

		// nodes are allocated breadth first, so just going through the list will guarantee the hierarchy is traversed in proper order
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