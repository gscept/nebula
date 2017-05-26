//------------------------------------------------------------------------------
//  depthstenciltarget.cc
//  (C) 2013 Gustav Sterbrant
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/multiplerendertarget.h"
#if (__DX11__ || __DX9__)
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::DepthStencilTarget, 'DSTG', Direct3D11::D3D11DepthStencilTarget);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::DepthStencilTarget, 'DSTG', Direct3D9::D3D9DepthStencilTarget);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::DepthStencilTarget, 'DSTG', OpenGL4::OGL4DepthStencilTarget);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::DepthStencilTarget, 'DSTG', Vulkan::VkDepthStencilTarget);
}
#else
#error "MultipleRenderTarget class not implemented on this platform!"
#endif
