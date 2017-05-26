#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::FeedbackBuffer
  
    A resource which holds elements (vertices) fed back from a transform feedback/stream output process
    
    (C) 2015-2016 Individual contributors, see AUTHORS file
*/    
#if __OGL4__
#include "coregraphics/ogl4/ogl4feedbackbuffer.h"
namespace CoreGraphics
{
class FeedbackBuffer : public OpenGL4::OGL4FeedbackBuffer
{
	__DeclareClass(FeedbackBuffer);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkfeedbackbuffer.h"
namespace CoreGraphics
{
class FeedbackBuffer : public Vulkan::VkFeedbackBuffer
{
	__DeclareClass(FeedbackBuffer);
};
}
#else
#error "FeedbackBuffer class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
