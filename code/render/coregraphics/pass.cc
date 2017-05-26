//------------------------------------------------------------------------------
//  pass.cc
//  (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/pass.h"

#if __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::Pass, 'FRBF', OpenGL4::OGL4Framebuffer);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::Pass, 'FRBF', Vulkan::VkPass);
}
#else
#error "FeedbackBuffer class not implemented on this platform!"
#endif

