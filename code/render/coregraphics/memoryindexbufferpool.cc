//------------------------------------------------------------------------------
//  memoryindexbufferloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "coregraphics/memoryindexbufferpool.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryIndexBufferPool, 'MIBP', Direct3D11::D3D11MemoryIndexBufferPool);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryIndexBufferPool, 'MIBP', OpenGL4::OGL4MemoryIndexBufferPool);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryIndexBufferPool, 'MIBP', Vulkan::VkMemoryIndexBufferPool);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryIndexBufferPool, 'MIBP', D3D9::D3D9MemoryIndexBufferPool);
}
#else
#error "MemoryIndexBufferLoader class not implemented on this platform!"
#endif
