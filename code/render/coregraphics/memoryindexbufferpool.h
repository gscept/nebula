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
class MemoryIndexBufferPool : public Direct3D11::D3D11MemoryIndexBufferLoader
{
	__DeclareClass(MemoryIndexBufferPool);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4memoryindexbufferloader.h"
namespace CoreGraphics
{
class MemoryIndexBufferPool : public OpenGL4::OGL4MemoryIndexBufferLoader
{
	__DeclareClass(MemoryIndexBufferPool);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkmemoryindexbufferpool.h"
namespace CoreGraphics
{
class MemoryIndexBufferPool : public Vulkan::VkMemoryIndexBufferPool
{
	__DeclareClass(MemoryIndexBufferPool);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9memoryindexbufferloader.h"
namespace CoreGraphics
{
class MemoryIndexBufferPool : public Direct3D9::D3D9MemoryIndexBufferLoader
{
    __DeclareClass(MemoryIndexBufferPool);
};
}
#else
#error "MemoryIndexBufferLoader class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

