#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::VertexLayout
    
    Describe the layout of vertices in a vertex buffer.
    
    (C) 2006 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11vertexlayout.h"
namespace CoreGraphics
{
class VertexLayout : public Direct3D11::D3D11VertexLayout
{
	__DeclareClass(VertexLayout);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4vertexlayout.h"
namespace CoreGraphics
{
class VertexLayout : public OpenGL4::OGL4VertexLayout
{
	__DeclareClass(VertexLayout);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkvertexlayout.h"
namespace CoreGraphics
{
class VertexLayout : public Vulkan::VkVertexLayout
{
	__DeclareClass(VertexLayout);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9vertexlayout.h"
namespace CoreGraphics
{
class VertexLayout : public Direct3D9::D3D9VertexLayout
{
    __DeclareClass(VertexLayout);
};
}
#else
#error "VertexLayout class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

