//------------------------------------------------------------------------------
//  instancerenderer.cc
//  (C) 2012 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "instancerenderer.h"
namespace Instancing
{
#if __DX11__
__ImplementClass(Instancing::InstanceRenderer, 'INRD', Instancing::D3D11InstanceRenderer);
#elif __OGL4__
__ImplementClass(Instancing::InstanceRenderer, 'INRD', Instancing::OGL4InstanceRenderer);
#elif __VULKAN__
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
