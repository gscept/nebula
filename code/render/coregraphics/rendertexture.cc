//------------------------------------------------------------------------------
//  rendertexture.cc
//  (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/rendertexture.h"

#if __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::RenderTexture, 'RETE', OpenGL4::OGL4RenderTexture);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::RenderTexture, 'RETE', Vulkan::VkRenderTexture);
}
#else
#error "FeedbackBuffer class not implemented on this platform!"
#endif

