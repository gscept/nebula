//------------------------------------------------------------------------------
// lightprobecontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "lightprobecontext.h"

namespace Graphics
{

__ImplementClass(Graphics::LightProbeContext, 'LPCO', Graphics::GraphicsContext);
//------------------------------------------------------------------------------
/**
*/
LightProbeContext::LightProbeContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
LightProbeContext::~LightProbeContext()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
ContextEntityId
LightProbeContext::Alloc()
{
	return ContextEntityId();
}

//------------------------------------------------------------------------------
/**
*/
void
LightProbeContext::Dealloc(ContextEntityId id)
{
}

} // namespace Graphics