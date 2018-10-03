#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::StreamShaderLoader
    
    Resource loader to setup a Shader object from a stream.
    
    (C) 2007 Radon Labs GmbH
    (C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11streamshaderloader.h"
namespace CoreGraphics
{
class ShaderPool : public Direct3D11::D3D11StreamShaderLoader
{
	__DeclareClass(ShaderPool);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4streamshaderloader.h"
namespace CoreGraphics
{
class ShaderPool : public OpenGL4::OGL4StreamShaderLoader
{
	__DeclareClass(ShaderPool);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkshaderpool.h"
namespace CoreGraphics
{
class ShaderPool : public Vulkan::VkShaderPool
{
	__DeclareClass(ShaderPool);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9streamshaderloader.h"
namespace CoreGraphics
{
class ShaderPool : public Direct3D9::D3D9StreamShaderLoader
{
    __DeclareClass(ShaderPool);
};
}
#else
#error "StreamShaderLoader class not implemented on this platform!"
#endif

    