#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::DepthStencilTarget
    
    A depth stencil target is a separate render target which is used for depth-stencil rendering.

    (C) 2013 Gustav Sterbrant
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11depthstenciltarget.h"
namespace CoreGraphics
{
class DepthStencilTarget : public Direct3D11::D3D11DepthStencilTarget
{
	__DeclareClass(DepthStencilTarget);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9depthstenciltarget.h"
namespace CoreGraphics
{
class DepthStencilTarget : public Direct3D9::D3D9DepthStencilTarget
{
	__DeclareClass(DepthStencilTarget);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4depthstenciltarget.h"
namespace CoreGraphics
{
class DepthStencilTarget : public OpenGL4::OGL4DepthStencilTarget
{
	__DeclareClass(DepthStencilTarget);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkdepthstenciltarget.h"
namespace CoreGraphics
{
class DepthStencilTarget : public Vulkan::VkDepthStencilTarget
{
	__DeclareClass(DepthStencilTarget);
};
}
#else
#error "DepthStencilTarget class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------