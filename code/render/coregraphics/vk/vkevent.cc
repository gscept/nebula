//------------------------------------------------------------------------------
//  vkevent.cc
//  (C) 2017 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "stdneb.h"
#include "vkevent.h"
#include "coregraphics/event.h"
#include "vkrenderdevice.h"
#include "coregraphics/config.h"
#include "vktypes.h"

namespace CoreGraphics
{

using namespace Vulkan;
VkEventAllocator eventAllocator(0x00FFFFFF);
//------------------------------------------------------------------------------
/**
*/
EventId
CreateEvent(const EventCreateInfo& info)
{
	VkEventCreateInfo createInfo = 
	{
		VK_STRUCTURE_TYPE_EVENT_CREATE_INFO,
		nullptr,
		0
	};
	Ids::Id32 id = eventAllocator.AllocObject();
	VkEventInfo& vkInfo = eventAllocator.Get<1>(id);

	VkDevice dev = VkRenderDevice::Instance()->GetCurrentDevice();
	vkCreateEvent(dev, &createInfo, nullptr, &vkInfo.event);

	vkInfo.numImageBarriers = 0;
	vkInfo.numBufferBarriers = 0;
	vkInfo.numMemoryBarriers = 0;
	vkInfo.leftDependency = VkTypes::AsVkPipelineFlags(info.leftDependency);
	vkInfo.rightDependency = VkTypes::AsVkPipelineFlags(info.rightDependency);
	eventAllocator.Get<0>(id) = dev;

	n_assert(info.renderTextureBarriers.Size() < MaxNumBarriers);
	n_assert(info.shaderRWTextures.Size() < MaxNumBarriers);
	n_assert(info.shaderRWBuffers.Size() < MaxNumBarriers);

	SizeT i;
	for (i = 0; i < info.renderTextureBarriers.Size(); i++)
	{
		vkInfo.imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkInfo.imageBarriers[i].pNext = nullptr;

		vkInfo.imageBarriers[i].srcAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<1>(info.renderTextureBarriers[i]));
		vkInfo.imageBarriers[i].dstAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<2>(info.renderTextureBarriers[i]));

		n_error("Implement RenderTexture method for getting Vk image");
		vkInfo.imageBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vkInfo.imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vkInfo.numImageBarriers++;
	}

	// make sure we have room...
	n_assert(info.shaderRWTextures.Size() < MaxNumBarriers - i);
	for (; i < info.shaderRWTextures.Size(); i++)
	{
		vkInfo.imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkInfo.imageBarriers[i].pNext = nullptr;

		vkInfo.imageBarriers[i].srcAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<1>(info.shaderRWTextures[i]));
		vkInfo.imageBarriers[i].dstAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<2>(info.shaderRWTextures[i]));

		n_error("Implement RenderTexture method for getting Vk image");
		vkInfo.imageBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vkInfo.imageBarriers[i].newLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		vkInfo.numImageBarriers++;
	}

	for (i = 0; i < info.shaderRWBuffers.Size(); i++)
	{
		vkInfo.bufferBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkInfo.bufferBarriers[i].pNext = nullptr;

		vkInfo.bufferBarriers[i].srcAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<1>(info.shaderRWBuffers[i]));
		vkInfo.bufferBarriers[i].dstAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<2>(info.shaderRWBuffers[i]));

		n_error("Implement RenderTexture method for getting Vk buffer");
		vkInfo.bufferBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.bufferBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.bufferBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.bufferBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.numBufferBarriers;
	}

	EventId eventId;
	eventId.id24 = id;
	eventId.id8 = EventIdType;
	return eventId;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyEvent(const EventId id)
{
	VkEventInfo& vkInfo = eventAllocator.Get<1>(id.id24);
	const VkDevice& dev = eventAllocator.Get<0>(id.id24);
	vkDestroyEvent(dev, vkInfo.event, nullptr);
	eventAllocator.DeallocObject(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void 
Signal(const EventId id, const CmdBufferId cmd, const BarrierDependency when)
{
	const VkEventInfo& vkInfo = eventAllocator.Get<1>(id.id24);
	vkCmdSetEvent(CommandBufferGetVk(cmd), vkInfo.event, VkTypes::AsVkPipelineFlags(when));
}

//------------------------------------------------------------------------------
/**
*/
void
Wait(const EventId id, const CmdBufferId cmd)
{
	const VkEventInfo& vkInfo = eventAllocator.Get<1>(id.id24);
	vkCmdWaitEvents(CommandBufferGetVk(cmd), 1, &vkInfo.event, vkInfo.leftDependency, vkInfo.rightDependency, vkInfo.numMemoryBarriers, vkInfo.memoryBarriers, vkInfo.numBufferBarriers, vkInfo.bufferBarriers, vkInfo.numImageBarriers, vkInfo.imageBarriers);
}

//------------------------------------------------------------------------------
/**
*/
void
Reset(const EventId id, const CmdBufferId cmd, const BarrierDependency when)
{
	const VkEventInfo& vkInfo = eventAllocator.Get<1>(id.id24);
	vkCmdResetEvent(CommandBufferGetVk(cmd), vkInfo.event, VkTypes::AsVkPipelineFlags(when));
}

} // namespace CoreGraphics
