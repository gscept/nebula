//------------------------------------------------------------------------------
// modelcontext.cc
// (C) 2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "modelcontext.h"
#include "resources/resourcemanager.h"
#include "nodes/modelnode.h"
#include "streammodelpool.h"
#include "graphics/graphicsserver.h"
#include "visibility/visibilitycontext.h"

#ifndef PUBLIC_BUILD
#include "dynui/im3d/im3dcontext.h"
#endif

using namespace Graphics;
using namespace Resources;
namespace Models
{

ModelContext::ModelContextAllocator ModelContext::modelContextAllocator;
_ImplementContext(ModelContext, ModelContext::modelContextAllocator);

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
	_CreateContext();

	__bundle.OnBeforeFrame = ModelContext::OnBeforeFrame;
	__bundle.OnWaitForWork = nullptr;
	__bundle.OnBeforeView = ModelContext::OnBeforeView;
	__bundle.OnAfterView = ModelContext::OnAfterView;
	__bundle.OnAfterFrame = ModelContext::OnAfterFrame;
	__bundle.StageBits = &ModelContext::__state.currentStage;
#ifndef PUBLIC_BUILD
    __bundle.OnRenderDebug = ModelContext::OnRenderDebug;
#endif
	ModelContext::__state.allowedRemoveStages = Graphics::OnBeforeFrameStage;
    ModelContext::__state.OnInstanceMoved = OnInstanceMoved;
	Graphics::GraphicsServer::Instance()->RegisterGraphicsContext(&__bundle, &__state);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::Setup(const Graphics::GraphicsEntityId gfxId, const Resources::ResourceName& name, const Util::StringAtom& tag)
{
	const ContextEntityId cid = GetContextId(gfxId);
    modelContextAllocator.Get<1>(cid.id) = ModelInstanceId::Invalid();
	
	ResourceCreateInfo info;
	info.resource = name;
	info.tag = tag;
	info.async = false;
	info.failCallback = nullptr;
	info.successCallback = [cid, gfxId](Resources::ResourceId id)
	{
		modelContextAllocator.Get<0>(cid.id) = id;
		ModelInstanceId& mdl = modelContextAllocator.Get<1>(cid.id);
		mdl = Models::CreateModelInstance(id);
		const Math::matrix44& pending = modelContextAllocator.Get<2>(cid.id);
		Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::InstanceTransform>(mdl.instance) = pending;
		Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::ObjectId>(mdl.instance) = gfxId.id;
	};

	modelContextAllocator.Get<0>(cid.id) = Models::CreateModel(info);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::ChangeModel(const Graphics::GraphicsEntityId gfxId, const Resources::ResourceName& name, const Util::StringAtom& tag)
{
	const ContextEntityId cid = GetContextId(gfxId);

	// clean up old stuff, but don't deallocate entity
	ModelId& rid = modelContextAllocator.Get<0>(cid.id);
	ModelInstanceId& mdl = modelContextAllocator.Get<1>(cid.id);

	if (mdl != ModelInstanceId::Invalid()) // actually deallocate current instance
		Models::DestroyModelInstance(mdl);
	if (rid != ModelId::Invalid()) // decrement model resource
		Models::DestroyModel(rid);
	mdl = ModelInstanceId::Invalid();

	ResourceCreateInfo info;
	info.resource = name;
	info.tag = tag;
	info.async = false;
	info.failCallback = nullptr;
	info.successCallback = [&mdl, rid, cid, gfxId](Resources::ResourceId id)
	{
		mdl = Models::CreateModelInstance(id);
		const Math::matrix44& pending = modelContextAllocator.Get<2>(cid.id);
		Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::InstanceTransform>(mdl.instance) = pending;
		Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::ObjectId>(mdl.instance) = gfxId.id;
	};

	rid = Models::CreateModel(info);
}

//------------------------------------------------------------------------------
/**
*/
const Models::ModelId 
ModelContext::GetModel(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return modelContextAllocator.Get<0>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Models::ModelInstanceId
ModelContext::GetModelInstance(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return modelContextAllocator.Get<1>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Models::ModelId 
ModelContext::GetModel(const Graphics::ContextEntityId id)
{
	return modelContextAllocator.Get<0>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const Models::ModelInstanceId
ModelContext::GetModelInstance(const Graphics::ContextEntityId id)
{
	return modelContextAllocator.Get<1>(id.id);
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
Math::matrix44
ModelContext::GetTransform(const Graphics::ContextEntityId id)
{
	ModelInstanceId& inst = modelContextAllocator.Get<1>(id.id);
	return modelContextAllocator.Get<2>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
Math::bbox 
ModelContext::GetBoundingBox(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	ModelInstanceId& inst = modelContextAllocator.Get<1>(cid.id);
	return Models::modelPool->modelInstanceAllocator.Get<4>(inst.instance);
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
*/
const Util::Array<Models::NodeType>& 
ModelContext::GetModelNodeTypes(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	ModelInstanceId& inst = modelContextAllocator.Get<1>(cid.id);
	return Models::modelPool->modelInstanceAllocator.Get<1>(inst.instance);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::ModelNode::Instance*>&
ModelContext::GetModelNodeInstances(const Graphics::ContextEntityId id)
{
	ModelInstanceId& inst = modelContextAllocator.Get<1>(id.id);
	return Models::modelPool->modelInstanceAllocator.Get<0>(inst.instance);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::NodeType>&
ModelContext::GetModelNodeTypes(const Graphics::ContextEntityId id)
{
	ModelInstanceId& inst = modelContextAllocator.Get<1>(id.id);
	return Models::modelPool->modelInstanceAllocator.Get<1>(inst.instance);
}

//------------------------------------------------------------------------------
/**
	Go through all models and apply their transforms
*/
void
ModelContext::OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime, const Timing::Time time, const Timing::Tick ticks)
{
	const Util::Array<ModelInstanceId>& instances = modelContextAllocator.GetArray<1>();
	const Util::Array<Math::matrix44>& transforms = Models::modelPool->modelInstanceAllocator.GetArray<StreamModelPool::InstanceTransform>();
	const Util::Array<Math::bbox>& modelBoxes = Models::modelPool->modelAllocator.GetArray<0>();
	Util::Array<Math::bbox>& instanceBoxes = Models::modelPool->modelInstanceAllocator.GetArray<StreamModelPool::InstanceBoundingBox>();
	Util::Array<Math::matrix44>& pending = modelContextAllocator.GetArray<2>();
	Util::Array<bool>& hasPending = modelContextAllocator.GetArray<3>();
	
	SizeT i;
	for (i = 0; i < instances.Size(); i++)
	{
		const ModelInstanceId& instance = instances[i];
		if (instance == ModelInstanceId::Invalid()) continue; // hmm, bad, should reorder and keep a 'last valid index'

		// get reference so we can include it in the pending
		Math::matrix44 transform = transforms[instance.instance];
		
		uint objectId = Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::ObjectId>(instance.instance);

		// if we have a pending transform, apply it and transform bounding box
		if (hasPending[i])
		{
			transform = pending[i];
			hasPending[i] = false;

			// transform the box
			instanceBoxes[instance.instance] = modelBoxes[instance.model];
			instanceBoxes[instance.instance].affine_transform(transform);

			// update the actual transform
			transforms[instance.instance] = transform;

			Util::Array<Models::ModelNode::Instance*>& nodes = Models::modelPool->modelInstanceAllocator.Get<0>(instance.instance);
			Util::Array<Models::NodeType>& types = Models::modelPool->modelInstanceAllocator.Get<1>(instance.instance);

			// nodes are allocated breadth first, so just going through the list will guarantee the hierarchy is traversed in proper order
			SizeT j;
			for (j = 0; j < nodes.Size(); j++)
			{
				Models::ModelNode::Instance* node = nodes[j];

				//if (!node->active)
				//	continue;

				Math::matrix44& parentTransform = transform;
				if (node->parent != nullptr && node->parent->node->type >= NodeHasTransform)
					parentTransform = static_cast<const TransformNode::Instance*>(node->parent)->modelTransform;

				if (types[j] >= NodeHasTransform)
				{
					TransformNode::Instance* tnode = static_cast<TransformNode::Instance*>(node);
					tnode->modelTransform = Math::matrix44::multiply(tnode->transform.getmatrix(), parentTransform);
					parentTransform = tnode->modelTransform;
					tnode->objectId = objectId;
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

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::OnRenderDebug(uint32_t flags)
{
    const Util::Array<ModelInstanceId>& instances = modelContextAllocator.GetArray<1>();    
    Util::Array<Math::bbox>& instanceBoxes = Models::modelPool->modelInstanceAllocator.GetArray<StreamModelPool::InstanceBoundingBox>();
    const Util::Array<Math::matrix44>& transforms = Models::modelPool->modelInstanceAllocator.GetArray<StreamModelPool::InstanceTransform>();
    const Util::Array<Math::bbox>& modelBoxes = Models::modelPool->modelAllocator.GetArray<0>();
    
    Math::float4 white(1.0f, 1.0f, 1.0f, 1.0f);
    Math::float4 gray(1.0f, 0.0f, 0.0f, 1.0f);
    int i, n;
    for (i = 0,n = instances.Size(); i<n ; i++)
    {
        const ModelInstanceId& instance = instances[i];
        if (instance == ModelInstanceId::Invalid()) continue;
        Im3d::Im3dContext::DrawBox(instanceBoxes[instance.instance], white, Im3d::CheckDepth|Im3d::Wireframe);
        Im3d::Im3dContext::DrawOrientedBox(transforms[instance.instance], modelBoxes[instance.model], gray, Im3d::CheckDepth);
    }
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::OnInstanceMoved(uint32_t toIndex, uint32_t fromIndex)
{
    if ((uint32_t)ModelContext::__state.entities.Size() > toIndex)
    {
        Visibility::ObservableContext::UpdateModelContextId(ModelContext::__state.entities[toIndex], toIndex);
    }
}

} // namespace Models