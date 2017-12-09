#pragma once
//------------------------------------------------------------------------------
/**
	@class CoreGraphics::ShaderVariable
	
	Provides direct access to a shader's global variable.
	The fastest way to change the value of a shader variable is to
	obtain a pointer to a shader variable once, and use it repeatedly
	to set new values.

	(C) 2007 Radon Labs GmbH
	(C) 2013-2016 Individual contributors, see AUTHORS file
*/
#if __DX11__
#include "coregraphics/d3d11/d3d11shadervariable.h"
namespace CoreGraphics
{
class ShaderVariable : public Direct3D11::D3D11ShaderVariable
{
	__DeclareClass(ShaderVariable);
};
}
#elif __OGL4__
#include "coregraphics/ogl4/ogl4shadervariable.h"
namespace CoreGraphics
{
class ShaderVariable : public OpenGL4::OGL4ShaderVariable
{
	__DeclareClass(ShaderVariable);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkshadervariable.h"
namespace CoreGraphics
{
class ShaderVariable : public Vulkan::VkShaderVariable
{
	__DeclareClass(ShaderVariable);
};
}
#elif __DX9__
#include "coregraphics/d3d9/d3d9shadervariable.h"
namespace CoreGraphics
{
class ShaderVariable : public Direct3D9::D3D9ShaderVariable
{
	__DeclareClass(ShaderVariable);
};
}
#else
#error "ShaderVariable class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------

