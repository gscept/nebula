#pragma once
//------------------------------------------------------------------------------
/**
	Adds a light representation to the graphics entity
	
	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphicscontext.h"
namespace Graphics
{
class LightContext : public GraphicsContext
{
	DeclareContext();
public:
	/// constructor
	LightContext();
	/// destructor
	virtual ~LightContext();

	/// get transform
	static const Math::matrix44& GetTransform(const Graphics::GraphicsEntityId id);

private:

	typedef Ids::IdAllocator<
		Math::matrix44,				// projection
		Math::matrix44				// view-transform
	> LightAllocator;
	static LightAllocator lightAllocator;

	/// allocate a new slice for this context
	static ContextEntityId Alloc();
	/// deallocate a slice
	static void Dealloc(ContextEntityId id);
};
} // namespace Graphics