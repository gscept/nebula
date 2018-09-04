//------------------------------------------------------------------------------
// lightcontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "lightcontext.h"
#include "graphics/graphicsserver.h"

namespace Graphics
{

ImplementContext(LightContext);
LightContext::LightAllocator LightContext::lightAllocator;
//------------------------------------------------------------------------------
/**
*/
LightContext::LightContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
LightContext::~LightContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
const Math::matrix44&
LightContext::GetTransform(const Graphics::GraphicsEntityId id)
{
	const ContextEntityId cid = GetContextId(id);
	return lightAllocator.Get<0>(cid.id);
}

//------------------------------------------------------------------------------
/**
*/
ContextEntityId
LightContext::Alloc()
{
	return ContextEntityId();
}

//------------------------------------------------------------------------------
/**
*/
void
LightContext::Dealloc(ContextEntityId id)
{
}

} // namespace Graphics