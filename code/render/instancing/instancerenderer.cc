//------------------------------------------------------------------------------
//  instancerenderer.cc
//  (C) 2012 Gustav Sterbrant
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "instancerenderer.h"
namespace Instancing
{
#if __VULKAN__
__ImplementClass(Instancing::InstanceRenderer, 'INRD', Vulkan::VkInstanceRenderer);
#else
#error "InstanceRenderer class not implemented on this platform!"
#endif

__ImplementSingleton(Instancing::InstanceRenderer);
//------------------------------------------------------------------------------
/**
*/
InstanceRenderer::InstanceRenderer()
{
	__ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
InstanceRenderer::~InstanceRenderer()
{
	__DestructSingleton;
}

} // namespace Instancing
