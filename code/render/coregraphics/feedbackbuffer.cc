//------------------------------------------------------------------------------
//  feedbackbuffer.cc
//  (C) 2015-2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "coregraphics/feedbackbuffer.h"

#if __OGL4__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::FeedbackBuffer, 'FEDB', OpenGL4::OGL4FeedbackBuffer);
}
#elif __VULKAN__
namespace CoreGraphics
{
__ImplementClass(CoreGraphics::FeedbackBuffer, 'FEDB', Vulkan::VkFeedbackBuffer);
}
#else
#error "FeedbackBuffer class not implemented on this platform!"
#endif

