#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::MemoryIndexBufferLoader
    
    Initialize an index buffer object from index data in memory.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11memoryindexbufferloader.h"
namespace CoreGraphics
{
class MemoryIndexBufferLoader : public Direct3D11::D3D11MemoryIndexBufferLoader
{
	__DeclareClass(MemoryIndexBufferLoader);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4memoryindexbufferloader.h"
namespace CoreGraphics
{
class MemoryIndexBufferLoader : public OpenGL4::OGL4MemoryIndexBufferLoader
{
	__DeclareClass(MemoryIndexBufferLoader);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkmemoryindexbufferloader.h"
namespace CoreGraphics
{
class MemoryIndexBufferLoader : public Vulkan::VkMemoryIndexBufferLoader
{
	__DeclareClass(MemoryIndexBufferLoader);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9memoryindexbufferloader.h"
namespace CoreGraphics
{
class MemoryIndexBufferLoader : public Direct3D9::D3D9MemoryIndexBufferLoader
{
    __DeclareClass(MemoryIndexBufferLoader);
};
}
#else
#error "MemoryIndexBufferLoader class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

