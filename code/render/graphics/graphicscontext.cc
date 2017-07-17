//------------------------------------------------------------------------------
// graphicscontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "graphicscontext.h"

namespace Graphics
{

__ImplementAbstractClass(Graphics::GraphicsContext, 'GRCO', Core::RefCounted);
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
GraphicsContext::OnBeforeFrame(const IndexT frameIndex, const Timing::Time frameTime)
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsContext::OnVisibilityReady(const IndexT frameIndex, const Timing::Time frameTime)
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsContext::OnBeforeView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime)
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsContext::OnAfterView(const Ptr<Graphics::View>& view, const IndexT frameIndex, const Timing::Time frameTime)
{
	// implement in subclass
}

//------------------------------------------------------------------------------
/**
*/
void
GraphicsContext::OnAfterFrame(const IndexT frameIndex, const Timing::Time frameTime)
{
	// implement in subclass
}

} // namespace Graphics