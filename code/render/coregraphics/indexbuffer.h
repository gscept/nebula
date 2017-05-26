#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::IndexBuffer
  
    A resource which holds an array of indices into an array of vertices.  
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11indexbuffer.h"
namespace CoreGraphics
{
	class IndexBuffer : public Direct3D11::D3D11IndexBuffer
	{
		__DeclareClass(IndexBuffer);
	};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4indexbuffer.h"
namespace CoreGraphics
{
class IndexBuffer : public OpenGL4::OGL4IndexBuffer
{
	__DeclareClass(IndexBuffer);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkindexbuffer.h"
namespace CoreGraphics
{
class IndexBuffer : public Vulkan::VkIndexBuffer
{
	__DeclareClass(IndexBuffer);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9indexbuffer.h"
namespace CoreGraphics
{
class IndexBuffer : public Direct3D9::D3D9IndexBuffer
{
    __DeclareClass(IndexBuffer);
};
}
#else
#error "IndexBuffer class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

