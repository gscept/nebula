#pragma once
//------------------------------------------------------------------------------
/**
	A model context bind a ModelInstance to a model, which allows it to be rendered using an .n3 file.
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphics/graphicscontext.h"
#include "core/singleton.h"
#include "resources/resourceid.h"
#include "materials/materialserver.h"
#include "model.h"
#include "nodes/modelnode.h"
namespace Models
{
class ModelContext : public Graphics::GraphicsContext
{
	DeclareContext();
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
	static const Models::ModelInstanceId GetModel(const Graphics::GraphicsEntityId id);

	/// set the transform for a model
	static void SetTransform(const Graphics::GraphicsEntityId id, const Math::matrix44& transform);
	/// get the transform for a model
	static Math::matrix44 GetTransform(const Graphics::GraphicsEntityId id);

	/// get model node instances
	static const Util::Array<Models::ModelNode::Instance*>& GetModelNodeInstances(const Graphics::GraphicsEntityId id);

	/// runs before frame is updated
	static void OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime);
	/// runs when visibility has finished processing 
	static void OnVisibilityReady(const IndexT frameIndex, const Timing::Time frameTime);
	/// runs before a specific view
	static void OnBeforeView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	/// runs after view is rendered
	static void OnAfterView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	/// runs after a frame is updated
	static void OnAfterFrame(const IndexT frameIndex, const Timing::Time frameTime);

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
	modelContextAllocator.DeallocObject(id.id);
}

} // namespace Models