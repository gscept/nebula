#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::Shader
  
    A shader object manages the entire render state required to render
    a mesh.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11shader.h"
namespace CoreGraphics
{
class Shader : public Direct3D11::D3D11Shader
{
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4shader.h"
namespace CoreGraphics
{
class Shader : public OpenGL4::OGL4Shader
{
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkshader.h"
namespace CoreGraphics
{
class Shader : public Vulkan::VkShader
{
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9shader.h"
namespace CoreGraphics
{
class Shader : public Direct3D9::D3D9Shader
{
};
}
#else
#error "Shader class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

