#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::ConstantBuffer

    A constant buffer is a buffer object which can be bound to 
    a shader in order to swap a chunk of shader variables.

    (C) 2015-2016 Individual contributors, see AUTHORS file
*/
#if __OGL4__
#include "coregraphics/ogl4/ogl4uniformbuffer.h"
namespace CoreGraphics
{
class ConstantBuffer : public OpenGL4::OGL4UniformBuffer
{
    __DeclareClass(ConstantBuffer);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkuniformbuffer.h"
namespace CoreGraphics
{
class ConstantBuffer : public Vulkan::VkUniformBuffer
{
	__DeclareClass(ConstantBuffer);
};
}
#else
#error "ShaderBuffer class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
