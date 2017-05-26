//------------------------------------------------------------------------------
// vkcmdevent.cc
// (C) 2016 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkcmdevent.h"
#include "coregraphics/vk/vkrenderdevice.h"
namespace Vulkan
{

__ImplementClass(Vulkan::VkCmdEvent, 'VKEV', Base::EventBase);
//------------------------------------------------------------------------------
/**
*/
VkCmdEvent::VkCmdEvent()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
VkCmdEvent::~VkCmdEvent()
{
	// empty
}

//------------------------------------------------------------------------------
/**
*/
void
VkCmdEvent::Setup()
{
	VkEventCreateInfo info =
	{
		VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
		NULL,
		0
	};
	VkResult res = vkCreateEvent(VkRenderDevice::dev, &info, NULL, &this->event);

	// if this event should be created signaled, just set it directly
	if (this->createSignaled) vkSetEvent(VkRenderDevice::dev, this->event);
}

//------------------------------------------------------------------------------
/**
*/
void
VkCmdEvent::Signal()
{
	vkCmdSetEvent(VkRenderDevice::mainCmdDrawBuffer, this->event, this->barrier->vkSrcFlags);
}

//------------------------------------------------------------------------------
/**
*/
void
VkCmdEvent::Wait()
{
	n_assert(this->barrier.isvalid());
	vkCmdWaitEvents(VkRenderDevice::mainCmdDrawBuffer, 1, &this->event, 
		this->barrier->vkSrcFlags, this->barrier->vkDstFlags, 
		this->barrier->vkNumMemoryBarriers, this->barrier->vkMemoryBarriers, 
		this->barrier->vkNumBufferBarriers, this->barrier->vkBufferBarriers, 
		this->barrier->vkNumImageBarriers, this->barrier->vkImageBarriers);
}

//------------------------------------------------------------------------------
/**
*/
void
VkCmdEvent::Reset()
{
	vkCmdResetEvent(VkRenderDevice::mainCmdDrawBuffer, this->event, this->barrier->vkDstFlags);
}

} // namespace Vulkan