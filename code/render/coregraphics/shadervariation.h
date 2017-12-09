#pragma once
//------------------------------------------------------------------------------
/**
	@class CoreGraphics::ShaderVariation
  
	A variation of a shader implements a specific feature set identified
	by a feature mask.
	
	(C) 2007 Radon Labs GmbH
	(C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11shadervariation.h"
namespace CoreGraphics
{
class ShaderVariation : public Direct3D11::D3D11ShaderVariation
{
	__DeclareClass(ShaderVariation);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4shaderprogram.h"
namespace CoreGraphics
{
class ShaderVariation : public OpenGL4::OGL4ShaderProgram
{
	__DeclareClass(ShaderVariation);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkshaderprogram.h"
namespace CoreGraphics
{
class ShaderVariation : public Vulkan::VkShaderProgram
{
	__DeclareClass(ShaderVariation);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9shadervariation.h"
namespace CoreGraphics
{
class ShaderVariation : public Direct3D9::D3D9ShaderVariation
{
	__DeclareClass(ShaderVariation);
};
}
#else
#error "ShaderVariation class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

