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
__ImplementClass(CoreGraphics::MemoryVertexBufferPool, 'MVBP', Direct3D11::D3D11MemoryVertexBufferPool);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryVertexBufferPool, 'MVBP', OpenGL4::OGL4MemoryVertexBufferPool);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryVertexBufferPool, 'MVBP', Vulkan::VkMemoryVertexBufferPool);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryVertexBufferPool, 'MVBP', Win360::D3D9MemoryVertexBufferPool);
}
#else
#error "MemoryVertexBufferPool class not implemented on this platform!"
#endif
