//------------------------------------------------------------------------------
// graphicscontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphicscontext.h"

namespace Graphics
{

__ImplementClass(Graphics::GraphicsContext, 'GRCO', Core::RefCounted);
//------------------------------------------------------------------------------
/**
*/
GraphicsContext::GraphicsContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
GraphicsContext::~GraphicsContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsContext::Update(const IndexT frameIndex, const Timing::Time frameTime)
{
	// do nothing, override in subclass
}

} // namespace Graphics