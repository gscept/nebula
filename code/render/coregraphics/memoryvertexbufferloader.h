#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::MemoryVertexBufferLoader
    
    Initialize a vertex buffer object from vertex data in memory.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11memoryvertexbufferloader.h"
namespace CoreGraphics
{
class MemoryVertexBufferLoader : public Direct3D11::D3D11MemoryVertexBufferLoader
{
	__DeclareClass(MemoryVertexBufferLoader);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4memoryvertexbufferloader.h"
namespace CoreGraphics
{
class MemoryVertexBufferLoader : public OpenGL4::OGL4MemoryVertexBufferLoader
{
	__DeclareClass(MemoryVertexBufferLoader);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkmemoryvertexbufferloader.h"
namespace CoreGraphics
{
class MemoryVertexBufferLoader : public Vulkan::VkMemoryVertexBufferLoader
{
	__DeclareClass(MemoryVertexBufferLoader);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9memoryvertexbufferloader.h"
namespace CoreGraphics
{
class MemoryVertexBufferLoader : public Direct3D9::D3D9MemoryVertexBufferLoader
{
    __DeclareClass(MemoryVertexBufferLoader);
};
}
#else
#error "MemoryVertexBufferLoader class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

