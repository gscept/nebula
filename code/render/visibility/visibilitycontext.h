#pragma once
//------------------------------------------------------------------------------
/**
	A VisibilityContext adds a module to a GraphicsEntity which makes it take part
	in visibility resolution. Most graphics entities should use this, but some entities,
	like the skybox, or UI elements, does not need to be checked for visibility. 

	The same goes for lights
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "core/refcounted.h"
#include "core/singleton.h"
#include "graphics/graphicscontext.h"
#include "coregraphics/batchgroup.h"
#include "util/hashtable.h"
#include "util/keyvaluepair.h"
#include "math/bbox.h"
#include "visibility.h"
#include "models/model.h"
namespace Visibility
{
class VisibilityContext : public Graphics::GraphicsContext
{
	__DeclareClass(VisibilityContext);
	__DeclareSingleton(VisibilityContext);
public:
	/// constructor
	VisibilityContext();
	/// destructor
	virtual ~VisibilityContext();

	/// setup
	void Setup(const Graphics::GraphicsEntityId id, VisibilityEntityType type);

	/// register entity
	void RegisterEntity(const Graphics::GraphicsEntityId entity) override;
	/// unregister entity
	void DeregisterEntity(const Graphics::GraphicsEntityId entity) override;

	/// runs before frame is updated
	void OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime);
private:



	Ids::IdAllocator<
		Math::matrix44,				// transform
		Math::bbox,					// bounding box
		Models::ModelInstanceId,	// model
		VisibilityEntityType		// type of visibility entity
	> visibilityContextAllocator;

	/// allocate a new slice for this context
	Graphics::ContextEntityId Alloc();
	/// deallocate a slice
	void Dealloc(Graphics::ContextEntityId id);
		
};

//------------------------------------------------------------------------------
/**
*/
inline Graphics::ContextEntityId
VisibilityContext::Alloc()
{
	return this->visibilityContextAllocator.AllocObject();
}

//------------------------------------------------------------------------------
/**
*/
inline void
VisibilityContext::Dealloc(Graphics::ContextEntityId id)
{
	this->visibilityContextAllocator.DeallocObject(id.id);
}

} // namespace Visibility