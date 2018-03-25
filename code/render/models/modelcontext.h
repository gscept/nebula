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
namespace Models
{
class ModelContext : public Graphics::GraphicsContext
{
	__DeclareClass(ModelContext);
	__DeclareSingleton(ModelContext);
public:
	/// constructor
	ModelContext();
	/// destructor
	virtual ~ModelContext();

	/// setup
	void Setup(const Graphics::GraphicsEntityId id, const Resources::ResourceName& name, const Util::StringAtom& tag);

	/// change model for existing entity
	void ChangeModel(const Graphics::GraphicsEntityId id, const Resources::ResourceName& name, const Util::StringAtom& tag);
	/// get model
	const Models::ModelInstanceId GetModel(const Graphics::GraphicsEntityId id);

	/// set the transform for a model
	void SetTransform(const Graphics::GraphicsEntityId id, const Math::matrix44 transform);

	/// runs before frame is updated
	void OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime);
	/// runs when visibility has finished processing 
	void OnVisibilityReady(const IndexT frameIndex, const Timing::Time frameTime);
	/// runs before a specific view
	void OnBeforeView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	/// runs after view is rendered
	void OnAfterView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime);
	/// runs after a frame is updated
	void OnAfterFrame(const IndexT frameIndex, const Timing::Time frameTime);

private:

	Ids::IdAllocator<
		ModelId,
		ModelInstanceId
	> modelContextAllocator;

	/// allocate a new slice for this context
	Graphics::ContextEntityId Alloc();
	/// deallocate a slice
	void Dealloc(Graphics::ContextEntityId id);

};

//------------------------------------------------------------------------------
/**
*/
inline Graphics::ContextEntityId
ModelContext::Alloc()
{
	return this->modelContextAllocator.AllocObject();
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelContext::Dealloc(Graphics::ContextEntityId id)
{
	this->modelContextAllocator.DeallocObject(id.id);
}

//------------------------------------------------------------------------------
/**
	Shorthand for adding a model to a graphics entity, Models::Attach
*/
inline void
ModelContextAttach(const Graphics::GraphicsEntityId entity, const Resources::ResourceName& res, const Util::StringAtom& tag)
{
	ModelContext::Instance()->RegisterEntity(entity);
	ModelContext::Instance()->Setup(entity, res, tag);
}

//------------------------------------------------------------------------------
/**
*/
inline void
ModelContextDetach(const Graphics::GraphicsEntityId entity)
{
	ModelContext::Instance()->DeregisterEntity(entity);
}

} // namespace Models