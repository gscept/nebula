//------------------------------------------------------------------------------
//  indexbuffer.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/indexbuffer.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::IndexBuffer, 'IDXB', Direct3D11::D3D11IndexBuffer);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::IndexBuffer, 'IDXB', OpenGL4::OGL4IndexBuffer);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::IndexBuffer, 'IDXB', Vulkan::VkIndexBuffer);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::IndexBuffer, 'IDXB', Direct3D9::D3D9IndexBuffer);
}
#else
#error "IndexBuffer class not implemented on this platform!"
#endif

