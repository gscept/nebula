//------------------------------------------------------------------------------
//  renderdevice.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/renderdevice.h"

namespace CoreGraphics
{
#if __DX11__
__ImplementClass(CoreGraphics::RenderDevice, 'RDVC', Direct3D11::D3D11RenderDevice);
#elif __OGL4__
__ImplementClass(CoreGraphics::RenderDevice, 'RDVC', OpenGL4::OGL4RenderDevice);
#elif __VULKAN__
__ImplementClass(CoreGraphics::RenderDevice, 'RDVC', Vulkan::VkRenderDevice);
#elif __DX9__
__ImplementClass(CoreGraphics::RenderDevice, 'RDVC', Direct3D9::D3D9RenderDevice);
#else
#error "RenderDevice class not implemented on this platform!"
#endif

__ImplementSingleton(CoreGraphics::RenderDevice);

//------------------------------------------------------------------------------
/**
*/
RenderDevice::RenderDevice()
{
    __ConstructSingleton;
}

//------------------------------------------------------------------------------
/**
*/
RenderDevice::~RenderDevice()
{
    __DestructSingleton;
}

} // namespace CoreGraphics
