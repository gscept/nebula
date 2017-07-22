//------------------------------------------------------------------------------
//  memorytextureloader.cc
//  (C) 2007 Radon Labs GmbH
//  (C) 2013-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/memorytexturepool.h"

#if __DX11__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryTexturePool, 'MTBL', Direct3D11::D3D11MemoryTextureLoader);
}
#elif __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryTexturePool, 'MTBL', OpenGL4::OGL4MemoryTextureLoader);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryTexturePool, 'MTBL', Vulkan::VkMemoryTexturePool);
}
#elif __DX9__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::MemoryTexturePool, 'MTBL', Direct3D9::D3D9MemoryTextureLoader);
}
#else
#error "MemoryIndexBufferLoader class not implemented on this platform!"
#endif
