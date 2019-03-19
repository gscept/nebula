#pragma once
//------------------------------------------------------------------------------
/**
	A model context bind a ModelInstance to a model, which allows it to be rendered using an .n3 file.
	
	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "core/singleton.h"
#include "resources/resourceid.h"
#include "materials/materialserver.h"
#include "model.h"
#include "nodes/modelnode.h"

namespace Jobs
{
struct JobFuncContext;
};

namespace Visibility
{
void VisibilitySortJob(const Jobs::JobFuncContext& ctx);
};

namespace Models
{
class ModelContext : public Graphics::GraphicsContext
{
	_DeclareContext();
public:
	/// constructor
	ModelContext();
	/// destructor
	virtual ~ModelContext();

	/// create context
	static void Create();

	/// setup
	static void Setup(const Graphics::GraphicsEntityId id, const Resources::ResourceName& name, const Util::StringAtom& tag);

	/// change model for existing entity
	static void ChangeModel(const Graphics::GraphicsEntityId id, const Resources::ResourceName& name, const Util::StringAtom& tag);
	/// get model
	static const Models::ModelId GetModel(const Graphics::GraphicsEntityId id);
	/// get model instance
	static const Models::ModelInstanceId GetModelInstance(const Graphics::GraphicsEntityId id);

	/// set the transform for a model
	static void SetTransform(const Graphics::GraphicsEntityId id, const Math::matrix44& transform);
	/// get the transform for a model
	static Math::matrix44 GetTransform(const Graphics::GraphicsEntityId id);
	/// get the transform for a model
	static Math::matrix44 GetTransform(const Graphics::ContextEntityId id);

	/// get model node instances
	static const Util::Array<Models::ModelNode::Instance*>& GetModelNodeInstances(const Graphics::GraphicsEntityId id);
	/// get model node types
	static const Util::Array<Models::NodeType>& GetModelNodeTypes(const Graphics::GraphicsEntityId id);

	/// runs before frame is updated
	static void OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime, const Timing::Time time, const Timing::Tick ticks);
	/// runs when visibility has finished processing 
	static void OnWaitForWork(const IndexT frameIndex, const Timing::Time frameTime);
	/// runs before a specific view
	static void OnBeforeView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	/// runs after view is rendered
	static void OnAfterView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	/// runs after a frame is updated
	static void OnAfterFrame(const IndexT frameIndex, const Timing::Time frameTime);
#ifndef PUBLIC_DEBUG    
	/// debug rendering
	static void OnRenderDebug(uint32_t flags);
#endif

private:
	typedef Ids::IdAllocator<
		ModelId,
		ModelInstanceId,
		Math::matrix44,			// pending transforms
		bool					// transform is dirty
	> ModelContextAllocator;
	static ModelContextAllocator modelContextAllocator;

	/// allocate a new slice for this context
	static Graphics::ContextEntityId Alloc();
	/// deallocate a slice
	static void Dealloc(Graphics::ContextEntityId id);

	friend void Visibility::VisibilitySortJob(const Jobs::JobFuncContext& ctx);

	/// get model
	static const Models::ModelId GetModel(const Graphics::ContextEntityId id);
	/// get model instance
	static const Models::ModelInstanceId GetModelInstance(const Graphics::ContextEntityId id);
	/// get model node instances
	static const Util::Array<Models::ModelNode::Instance*>& GetModelNodeInstances(const Graphics::ContextEntityId id);
	/// get model node instances
	static const Util::Array<Models::NodeType>& GetModelNodeTypes(const Graphics::ContextEntityId id);
};

//------------------------------------------------------------------------------
/**
*/
inline Graphics::ContextEntityId
ModelContext::Alloc()
{
	return modelContextAllocator.AllocObject();
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelContext::Dealloc(Graphics::ContextEntityId id)
{
	// clean up old stuff, but don't deallocate entity
	ModelId& rid = modelContextAllocator.Get<0>(id.id);
	ModelInstanceId& mdl = modelContextAllocator.Get<1>(id.id);

	if (mdl != ModelInstanceId::Invalid()) // actually deallocate current instance
		Models::DestroyModelInstance(mdl);
	if (rid != ModelId::Invalid()) // decrement model resource
		Models::DestroyModel(rid);
	mdl = ModelInstanceId::Invalid();

	modelContextAllocator.DeallocObject(id.id);
}

} // namespace Models