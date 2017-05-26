//------------------------------------------------------------------------------
//  vertexbuffer.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/vertexbuffer.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::VertexBuffer, 'VTXB', Direct3D11::D3D11VertexBuffer);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::VertexBuffer, 'VTXB', OpenGL4::OGL4VertexBuffer);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::VertexBuffer, 'VTXB', Vulkan::VkVertexBuffer);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::VertexBuffer, 'VTXB', Direct3D9::D3D9VertexBuffer);
}
#else
#error "VertexBuffer class not implemented on this platform!"
#endif

