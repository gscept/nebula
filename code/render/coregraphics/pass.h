#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::Pass
  
    A pass describe a set of textures used for rendering
    
    (C) 2015 Individual contributors, see AUTHORS file
*/    
#if __OGL4__
#include "coregraphics/ogl4/ogl4pass.h"
namespace CoreGraphics
{
class Pass : public OpenGL4::OGL4Framebuffer
{
	__DeclareClass(Pass);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkpass.h"
namespace CoreGraphics
{
class Pass : public Vulkan::VkPass
{
	__DeclareClass(Pass);
};
}
#else
#error "FeedbackBuffer class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
