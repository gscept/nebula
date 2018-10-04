#pragma once
//------------------------------------------------------------------------------
/**
	Adds a light probe component to graphics entities
	
	(C)2017-2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "graphicscontext.h"
namespace Graphics
{
class LightProbeContext : public GraphicsContext
{
	DeclareContext();
public:
	/// constructor
	LightProbeContext();
	/// destructor
	virtual ~LightProbeContext();

	/// get transform
	static const Math::matrix44& GetTransform(const Graphics::GraphicsEntityId id);
private:

	typedef Ids::IdAllocator<
		Math::matrix44,				// projection
		Math::matrix44				// view-transform
	> LightProbeAllocator;
	static LightProbeAllocator lightProbeAllocator;

	/// allocate a new slice for this context
	static ContextEntityId Alloc();
	/// deallocate a slice
	static void Dealloc(ContextEntityId id);
};
} // namespace Graphics