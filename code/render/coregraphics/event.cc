//------------------------------------------------------------------------------
//  event.cc
//  (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/event.h"

#if __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::Event, 'PASS', OpenGL4::OGL4Event);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::Event, 'PASS', Vulkan::VkCmdEvent);
}
#else
#error "FeedbackBuffer class not implemented on this platform!"
#endif

