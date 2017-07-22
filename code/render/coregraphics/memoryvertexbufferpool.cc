//------------------------------------------------------------------------------
//  memoryvertexbufferloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/memoryvertexbufferpool.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryVertexBufferPool, 'MVBL', Direct3D11::D3D11MemoryVertexBufferLoader);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryVertexBufferPool, 'MVBL', OpenGL4::OGL4MemoryVertexBufferLoader);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryVertexBufferPool, 'MVBL', Vulkan::VkMemoryVertexBufferPool);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryVertexBufferPool, 'MVBL', Win360::D3D9MemoryVertexBufferLoader);
}
#else
#error "MemoryVertexBufferLoader class not implemented on this platform!"
#endif
