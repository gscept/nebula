#pragma once
//------------------------------------------------------------------------------
/**
    @class CoreGraphics::Event
  
    An event is an explicit synchronization variable used to notify
	the render path that a sequence of work is done.
    
    (C) 2015 Individual contributors, see AUTHORS file
*/    
#if __OGL4__
#include "coregraphics/ogl4/ogl4event.h"
namespace CoreGraphics
{
class Event : public OpenGL4::OGL4Event
{
	__DeclareClass(Pass);
};
}
#elif __VULKAN__
#include "coregraphics/vk/vkcmdevent.h"
namespace CoreGraphics
{
class Event : public Vulkan::VkCmdEvent
{
	__DeclareClass(Event);
};
}
#else
#error "FeedbackBuffer class not implemented on this platform!"
#endif
//------------------------------------------------------------------------------
