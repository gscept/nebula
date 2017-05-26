#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::RenderTexture
  
    A render texture implements a renderable surface.
    
    (C) 2015 Individual contributors, see AUTHORS file
*/    
#if __OGL4__
#include "coregraphics/ogl4/ogl4rendertexture.h"
namespace CoreGraphics
{
class RenderTexture : public OpenGL4::OGL4RenderTexture
{
	__DeclareClass(RenderTexture);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkrendertexture.h"
namespace CoreGraphics
{
class RenderTexture : public Vulkan::VkRenderTexture
{
	__DeclareClass(RenderTexture);
};
}
#else
#error "FeedbackBuffer class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
