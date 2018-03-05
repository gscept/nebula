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
private:

	typedef Ids::Id32 MaterialTypeId;
	typedef CoreGraphics::BatchGroup::Code MaterialBatchId;
	typedef Ids::Id32 MaterialInstanceId;
	typedef Ids::Id32 ModelResourceId;
	typedef Ids::Id32 ModelNodeResourceId;
	typedef Ids::Id32 ModelNodeInstanceId;

	template <class KEY, class VALUE>
	struct VisibilityLevel
	{
		bool anyVisible;
		Util::HashTable<KEY, VALUE> level;
	};

	/// the visibility data structure, the entity id is the observer list
	typedef Util::HashTable<Graphics::GraphicsEntityId,
		VisibilityLevel<MaterialTypeId,
		VisibilityLevel<MaterialBatchId,
		VisibilityLevel<MaterialInstanceId,
		VisibilityLevel<ModelResourceId,
		VisibilityLevel<ModelNodeResourceId, Util::Array<ModelNodeInstanceId>
		>>>>>> VisibilityMatrix;		

	Ids::IdAllocator<
		Math::matrix44,			// transform
		Math::bbox,				// bounding box
		VisibilityEntityType	// type of visibility entity
	> visibilityContextAllocator;

	/// allocate a new slice for this context
	Ids::Id32 Alloc();
	/// deallocate a slice
	void Dealloc(Ids::Id32 id);
		
};

//------------------------------------------------------------------------------
/**
*/
inline Ids::Id32
VisibilityContext::Alloc()
{
	return this->visibilityContextAllocator.AllocObject();
}

//------------------------------------------------------------------------------
/**
*/
inline void
VisibilityContext::Dealloc(Ids::Id32 id)
{
	this->visibilityContextAllocator.DeallocObject(id);
}

} // namespace Visibility