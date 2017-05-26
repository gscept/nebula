//------------------------------------------------------------------------------
//  vertexlayout.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/vertexlayout.h"
#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::VertexLayout, 'VTXL', Direct3D11::D3D11VertexLayout);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::VertexLayout, 'VTXL', OpenGL4::OGL4VertexLayout);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::VertexLayout, 'VTXL', Vulkan::VkVertexLayout);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::VertexLayout, 'VTXL', Direct3D9::D3D9VertexLayout);
}
#else
#error "VertexLayout class not implemented on this platform!"
#endif

