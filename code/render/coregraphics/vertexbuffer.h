#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::VertexBuffer
  
    A resource which holds an array of vertices.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/    
#if __DX11__
#include "coregraphics/d3d11/d3d11vertexbuffer.h"
namespace CoreGraphics
{
class VertexBuffer : public Direct3D11::D3D11VertexBuffer
{
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4vertexbuffer.h"
namespace CoreGraphics
{
class VertexBuffer : public OpenGL4::OGL4VertexBuffer
{
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkvertexbuffer.h"
namespace CoreGraphics
{
class VertexBuffer : public Vulkan::VkVertexBuffer
{
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9vertexbuffer.h"
namespace CoreGraphics
{
class VertexBuffer : public Direct3D9::D3D9VertexBuffer
{
};
}
#else
#error "VertexBuffer class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
