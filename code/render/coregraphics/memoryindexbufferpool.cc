//------------------------------------------------------------------------------
//  memoryindexbufferloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/memoryindexbufferpool.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryIndexBufferPool, 'MIBL', Direct3D11::D3D11MemoryIndexBufferLoader);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryIndexBufferPool, 'MIBL', OpenGL4::OGL4MemoryIndexBufferLoader);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryIndexBufferPool, 'MIBL', Vulkan::VkMemoryIndexBufferPool);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryIndexBufferPool, 'MIBL', Win360::D3D9MemoryIndexBufferLoader);
}
#else
#error "MemoryIndexBufferLoader class not implemented on this platform!"
#endif
