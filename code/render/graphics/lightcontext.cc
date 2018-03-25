//------------------------------------------------------------------------------
// lightcontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "lightcontext.h"

namespace Graphics
{

__ImplementClass(Graphics::LightContext, 'LICO', Graphics::GraphicsContext);
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