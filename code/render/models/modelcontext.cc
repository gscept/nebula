//------------------------------------------------------------------------------
// modelcontext.cc
// (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "modelcontext.h"
#include "resources/resourceserver.h"
#include "nodes/modelnode.h"
#include "streammodelpool.h"
#include "graphics/graphicsserver.h"
#include "visibility/visibilitycontext.h"
#include "profiling/profiling.h"
#include "graphics/cameracontext.h"

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

	__bundle.OnBegin = ModelContext::UpdateTransforms;
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
ModelContext::Setup(const Graphics::GraphicsEntityId gfxId, const Resources::ResourceName& name, const Util::StringAtom& tag, std::function<void()> finishedCallback)
{
	const ContextEntityId cid = GetContextId(gfxId);
    modelContextAllocator.Get<Model_InstanceId>(cid.id) = ModelInstanceId::Invalid();
	
	ResourceCreateInfo info;
	info.resource = name;
	info.tag = tag;
	info.async = true;
	info.successCallback = [cid, gfxId, finishedCallback](Resources::ResourceId mid)
	{
		modelContextAllocator.Get<Model_Id>(cid.id) = mid;
		ModelInstanceId& mdl = modelContextAllocator.Get<Model_InstanceId>(cid.id);
		mdl = Models::CreateModelInstance(mid);
		const Math::mat4& pending = modelContextAllocator.Get<Model_Transform>(cid.id);
		Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::InstanceTransform>(mdl.instance) = pending;
		Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::ObjectId>(mdl.instance) = gfxId.id;
		if (finishedCallback != nullptr)
			finishedCallback();
	};
	info.failCallback = info.successCallback;

	ModelId mid = Models::CreateModel(info);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::ChangeModel(const Graphics::GraphicsEntityId gfxId, const Resources::ResourceName& name, const Util::StringAtom& tag, std::function<void()> finishedCallback)
{
	const ContextEntityId cid = GetContextId(gfxId);

	// clean up old stuff, but don't deallocate entity
	ModelId& rid = modelContextAllocator.Get<Model_Id>(cid.id);
	ModelInstanceId& mdl = modelContextAllocator.Get<Model_InstanceId>(cid.id);

	if (mdl != ModelInstanceId::Invalid()) // actually deallocate current instance
		Models::DestroyModelInstance(mdl);
	if (rid != ModelId::Invalid()) // decrement model resource
		Models::DestroyModel(rid);
	mdl = ModelInstanceId::Invalid();

	ResourceCreateInfo info;
	info.resource = name;
	info.tag = tag;
	info.async = false;
	info.successCallback = [&mdl, cid, gfxId, finishedCallback](Resources::ResourceId mid)
	{
		modelContextAllocator.Get<Model_Id>(cid.id) = mid;
		ModelInstanceId& mdl = modelContextAllocator.Get<Model_InstanceId>(cid.id);
		const Math::mat4& pending = modelContextAllocator.Get<Model_Transform>(cid.id);
		Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::InstanceTransform>(mdl.instance) = pending;
		Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::ObjectId>(mdl.instance) = gfxId.id;
		finishedCallback();
	};
	info.failCallback = info.successCallback;

	rid = Models::CreateModel(info);
}

//------------------------------------------------------------------------------
/**
*/
const Models::ModelId 
ModelContext::GetModel(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return modelContextAllocator.Get<Model_Id>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Models::ModelInstanceId
ModelContext::GetModelInstance(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return modelContextAllocator.Get<Model_InstanceId>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
const Models::ModelId 
ModelContext::GetModel(const Graphics::ContextEntityId id)
{
	return modelContextAllocator.Get<Model_Id>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
const Models::ModelInstanceId
ModelContext::GetModelInstance(const Graphics::ContextEntityId id)
{
	return modelContextAllocator.Get<Model_InstanceId>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::SetTransform(const Graphics::GraphicsEntityId id, const Math::mat4& transform)
{
	const ContextEntityId cid = GetContextId(id);
	Math::mat4& pending = modelContextAllocator.Get<Model_Transform>(cid.id);
	bool& hasPending = modelContextAllocator.Get<Model_Dirty>(cid.id);
	pending = transform;
	hasPending = true;
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
ModelContext::GetTransform(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return modelContextAllocator.Get<Model_Transform>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
Math::mat4
ModelContext::GetTransform(const Graphics::ContextEntityId id)
{
	ModelInstanceId& inst = modelContextAllocator.Get<Model_InstanceId>(id.id);
	return modelContextAllocator.Get<Model_Transform>(id.id);
}

//------------------------------------------------------------------------------
/**
*/
Math::bbox 
ModelContext::GetBoundingBox(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	ModelInstanceId& inst = modelContextAllocator.Get<Model_InstanceId>(cid.id);
	return Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::InstanceBoundingBox>(inst.instance);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::ModelNode::Instance*>& 
ModelContext::GetModelNodeInstances(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	ModelInstanceId& inst = modelContextAllocator.Get<Model_InstanceId>(cid.id);
	return Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::ModelNodeInstances>(inst.instance);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::NodeType>& 
ModelContext::GetModelNodeTypes(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	ModelInstanceId& inst = modelContextAllocator.Get<Model_InstanceId>(cid.id);
	return Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::ModelNodeTypes>(inst.instance);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::ModelNode::Instance*>&
ModelContext::GetModelNodeInstances(const Graphics::ContextEntityId id)
{
	ModelInstanceId& inst = modelContextAllocator.Get<Model_InstanceId>(id.id);
	return Models::modelPool->modelInstanceAllocator.Get<Model_Id>(inst.instance);
}

//------------------------------------------------------------------------------
/**
*/
const Util::Array<Models::NodeType>&
ModelContext::GetModelNodeTypes(const Graphics::ContextEntityId id)
{
	ModelInstanceId& inst = modelContextAllocator.Get<Model_InstanceId>(id.id);
	return Models::modelPool->modelInstanceAllocator.Get<Model_InstanceId>(inst.instance);
}

//------------------------------------------------------------------------------
/**
	Go through all models and apply their transforms
*/
void
ModelContext::UpdateTransforms(const Graphics::FrameContext& ctx)
{
	N_SCOPE(UpdateTransforms, Models);
	const Util::Array<ModelInstanceId>& instances = modelContextAllocator.GetArray<Model_InstanceId>();
	const Util::Array<Math::mat4>& transforms = Models::modelPool->modelInstanceAllocator.GetArray<StreamModelPool::InstanceTransform>();
	const Util::Array<Math::bbox>& modelBoxes = Models::modelPool->modelAllocator.GetArray<Models::StreamModelPool::ModelBoundingBox>();
	Util::Array<Math::bbox>& instanceBoxes = Models::modelPool->modelInstanceAllocator.GetArray<StreamModelPool::InstanceBoundingBox>();
	Util::Array<Math::mat4>& pending = modelContextAllocator.GetArray<Model_Transform>();
	Util::Array<bool>& hasPending = modelContextAllocator.GetArray<Model_Dirty>();

	// get the lod camera
	Graphics::GraphicsEntityId lodCamera = Graphics::CameraContext::GetLODCamera();
	const Math::mat4& cameraTransform = inverse(Graphics::CameraContext::GetTransform(lodCamera));
	
	SizeT i;
	for (i = 0; i < instances.Size(); i++)
	{
		const ModelInstanceId& instance = instances[i];
		if (instance == ModelInstanceId::Invalid()) 
			continue; // hmm, bad, should reorder and keep a 'last valid index'

		Util::Array<Models::ModelNode::Instance*>& nodes = Models::modelPool->modelInstanceAllocator.Get<Models::StreamModelPool::ModelNodeInstances>(instance.instance);
		Util::Array<Models::NodeBits>& bits = Models::modelPool->modelInstanceAllocator.Get<Models::StreamModelPool::ModelNodeBits>(instance.instance);
		if (hasPending[i])
		{
			uint objectId = Models::modelPool->modelInstanceAllocator.Get<StreamModelPool::ObjectId>(instance.instance);

			// copy matrix pending matrix
			Math::mat4 transform = pending[i];
			hasPending[i] = false;

			// transform the box
			instanceBoxes[instance.instance] = modelBoxes[instance.model];
			instanceBoxes[instance.instance].affine_transform(transform);

			// update the actual transform
			transforms[instance.instance] = transform;

			// nodes are allocated breadth first, so just going through the list will guarantee the hierarchy is traversed in proper order
			SizeT j;
			for (j = 0; j < nodes.Size(); j++)
			{
				Models::ModelNode::Instance* node = nodes[j];

				//if (!node->active)
				//	continue;

				Math::mat4 parentTransform = transform;
				if (node->parent != nullptr && (node->parent->node->bits & HasTransformBit) == HasTransformBit)
					parentTransform = reinterpret_cast<const TransformNode::Instance*>(node->parent)->modelTransform;

				if ((bits[j] & HasTransformBit) == HasTransformBit)
				{
					TransformNode::Instance* tnode = reinterpret_cast<TransformNode::Instance*>(node);
					tnode->modelTransform = tnode->transform.getmatrix() * parentTransform;
					tnode->invModelTransform = inverse(tnode->modelTransform);
					parentTransform = tnode->modelTransform;
					tnode->objectId = objectId;

				}
				if ((bits[j] & HasStateBit) == HasStateBit)
				{
					ShaderStateNode::Instance* snode = reinterpret_cast<ShaderStateNode::Instance*>(node);

					// copy bounding box from parent, then transform
					snode->boundingBox = node->node->boundingBox;
					snode->boundingBox.affine_transform(snode->modelTransform);
				}
				transform = parentTransform;
			}
		}

		// calculate view vector to calculate LOD
		Math::vec4 viewVector = cameraTransform.position - transforms[instance.instance].position;
		float viewDistance = length(viewVector);
		float textureLod = viewDistance - 38.5f;

		// nodes are allocated breadth first, so just going through the list will guarantee the hierarchy is traversed in proper order
		SizeT j;
		for (j = 0; j < nodes.Size(); j++)
		{
			Models::ModelNode::Instance* node = nodes[j];
			if ((bits[j] & HasTransformBit) == HasTransformBit)
			{
				TransformNode::Instance* tnode = reinterpret_cast<TransformNode::Instance*>(node);
				TransformNode* pnode = reinterpret_cast<TransformNode*>(tnode->node);

				// perform LOD testing
				if (pnode->useLodDistances)
				{
					if (viewDistance >= pnode->minDistance && viewDistance < pnode->maxDistance)
						tnode->active = true;
					else
						tnode->active = false;
					tnode->lodFactor = (viewDistance - (pnode->minDistance + 1.5f)) / (pnode->maxDistance - (pnode->minDistance + 1.5f));
				}
				else
					tnode->lodFactor = 0.0f;
			}

			if ((bits[j] & HasStateBit) == HasStateBit)
			{
				ShaderStateNode::Instance* snode = reinterpret_cast<ShaderStateNode::Instance*>(node);
				ShaderStateNode* pnode = reinterpret_cast<ShaderStateNode*>(node->node);
				if (!snode->active)
					continue;

				snode->SetDirty(true);
				pnode->SetMaxLOD(textureLod);
			}
		}
	}
}

//------------------------------------------------------------------------------
/**
*/
void
ModelContext::OnRenderDebug(uint32_t flags)
{
    const Util::Array<ModelInstanceId>& instances = modelContextAllocator.GetArray<Model_InstanceId>();    
    Util::Array<Math::bbox>& instanceBoxes = Models::modelPool->modelInstanceAllocator.GetArray<StreamModelPool::InstanceBoundingBox>();
    const Util::Array<Math::mat4>& transforms = Models::modelPool->modelInstanceAllocator.GetArray<StreamModelPool::InstanceTransform>();
    const Util::Array<Math::bbox>& modelBoxes = Models::modelPool->modelAllocator.GetArray<Model_Id>();
    
    Math::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);
    int i, n;
    for (i = 0,n = instances.Size(); i<n ; i++)
    {
        const ModelInstanceId& instance = instances[i];
        if (instance == ModelInstanceId::Invalid()) continue;
		CoreGraphics::RenderShape shape;
		shape.SetupSimpleShape(CoreGraphics::RenderShape::Box, CoreGraphics::RenderShape::RenderFlag(CoreGraphics::RenderShape::CheckDepth | CoreGraphics::RenderShape::Wireframe), transforms[instance.instance], white);
		CoreGraphics::ShapeRenderer::Instance()->AddShape(shape);
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