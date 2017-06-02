//------------------------------------------------------------------------------
// visibilitycontext.cc
// (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "visibilitycontext.h"

namespace Visibility
{

__ImplementClass(Visibility::VisibilityContext, 'VICX', Graphics::GraphicsContext);
__ImplementSingleton(Visibility::VisibilityContext);
//------------------------------------------------------------------------------
/**
*/
VisibilityContext::VisibilityContext()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
VisibilityContext::~VisibilityContext()
{
	__DestructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
int64_t
VisibilityContext::RegisterEntity(const int64_t& entity)
{
	return GraphicsContext::RegisterEntity(entity);
}

//------------------------------------------------------------------------------
/**
*/
void
VisibilityContext::UnregisterEntity(const int64_t& entity)
{

}

} // namespace Visibility