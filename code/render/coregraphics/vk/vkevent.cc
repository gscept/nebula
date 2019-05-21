//------------------------------------------------------------------------------
//  vkevent.cc
//  (C)2017-2018 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkevent.h"
#include "coregraphics/event.h"
#include "vkgraphicsdevice.h"
#include "coregraphics/config.h"
#include "vktypes.h"
#include "vkshaderrwtexture.h"
#include "vkshaderrwbuffer.h"
#include "vkrendertexture.h"

#ifdef CreateEvent
#pragma push_macro("CreateEvent")
#undef CreateEvent
#endif

namespace Vulkan
{
VkEventAllocator eventAllocator(0x00FFFFFF);
}

namespace CoreGraphics
{

using namespace Vulkan;
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
	Ids::Id32 id = eventAllocator.Alloc();
	VkEventInfo& vkInfo = eventAllocator.Get<1>(id);

	VkDevice dev = Vulkan::GetCurrentDevice();
	vkCreateEvent(dev, &createInfo, nullptr, &vkInfo.event);

	vkInfo.numImageBarriers = 0;
	vkInfo.numBufferBarriers = 0;
	vkInfo.numMemoryBarriers = 0;
	vkInfo.leftDependency = VkTypes::AsVkPipelineFlags(info.leftDependency);
	vkInfo.rightDependency = VkTypes::AsVkPipelineFlags(info.rightDependency);
	eventAllocator.Get<0>(id) = dev;

	n_assert(info.renderTextures.Size() < EventMaxNumBarriers);
	n_assert(info.shaderRWTextures.Size() < EventMaxNumBarriers);
	n_assert(info.shaderRWBuffers.Size() < EventMaxNumBarriers);

	SizeT i;
	for (i = 0; i < info.renderTextures.Size(); i++)
	{
		vkInfo.imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkInfo.imageBarriers[i].pNext = nullptr;

		vkInfo.imageBarriers[i].srcAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<4>(info.renderTextures[i]));
		vkInfo.imageBarriers[i].dstAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<5>(info.renderTextures[i]));

		const ImageSubresourceInfo& subres = std::get<1>(info.renderTextures[i]);
		vkInfo.imageBarriers[i].subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.aspect);
		vkInfo.imageBarriers[i].subresourceRange.baseMipLevel = subres.mip;
		vkInfo.imageBarriers[i].subresourceRange.levelCount = subres.mipCount;
		vkInfo.imageBarriers[i].subresourceRange.baseArrayLayer = subres.layer;
		vkInfo.imageBarriers[i].subresourceRange.layerCount = subres.layerCount;
		vkInfo.imageBarriers[i].image = RenderTextureGetVkImage(std::get<0>(info.renderTextures[i]));
		vkInfo.imageBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].oldLayout = VkTypes::AsVkImageLayout(std::get<2>(info.renderTextures[i]));
		vkInfo.imageBarriers[i].newLayout = VkTypes::AsVkImageLayout(std::get<3>(info.renderTextures[i]));
		vkInfo.numImageBarriers++;
	}

	// make sure we have room...
	n_assert(info.shaderRWTextures.Size() < EventMaxNumBarriers - i);
	for (; i < info.shaderRWTextures.Size(); i++)
	{
		vkInfo.imageBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkInfo.imageBarriers[i].pNext = nullptr;

		vkInfo.imageBarriers[i].srcAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<4>(info.shaderRWTextures[i]));
		vkInfo.imageBarriers[i].dstAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<5>(info.shaderRWTextures[i]));

		const ImageSubresourceInfo& subres = std::get<1>(info.shaderRWTextures[i]);
		vkInfo.imageBarriers[i].subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.aspect);
		vkInfo.imageBarriers[i].subresourceRange.baseMipLevel = subres.mip;
		vkInfo.imageBarriers[i].subresourceRange.levelCount = subres.mipCount;
		vkInfo.imageBarriers[i].subresourceRange.baseArrayLayer = subres.layer;
		vkInfo.imageBarriers[i].subresourceRange.layerCount = subres.layerCount;
		vkInfo.imageBarriers[i].image = ShaderRWTextureGetVkImage(std::get<0>(info.shaderRWTextures[i]));
		vkInfo.imageBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[i].oldLayout = VkTypes::AsVkImageLayout(std::get<2>(info.shaderRWTextures[i]));
		vkInfo.imageBarriers[i].newLayout = VkTypes::AsVkImageLayout(std::get<3>(info.shaderRWTextures[i]));
		vkInfo.numImageBarriers++;
	}

	for (i = 0; i < info.shaderRWBuffers.Size(); i++)
	{
		vkInfo.bufferBarriers[i].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkInfo.bufferBarriers[i].pNext = nullptr;

		vkInfo.bufferBarriers[i].srcAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<1>(info.shaderRWBuffers[i]));
		vkInfo.bufferBarriers[i].dstAccessMask = VkTypes::AsVkResourceAccessFlags(std::get<2>(info.shaderRWBuffers[i]));

		vkInfo.bufferBarriers[i].buffer = ShaderRWBufferGetVkBuffer(std::get<0>(info.shaderRWBuffers[i]));
		vkInfo.bufferBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.bufferBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.bufferBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.bufferBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.numBufferBarriers++;
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
	eventAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void 
EventSignal(const EventId id, const CoreGraphicsQueueType queue)
{
	CoreGraphics::SignalEvent(id, queue);
}

//------------------------------------------------------------------------------
/**
*/
void
EventWait(const EventId id, const CoreGraphicsQueueType queue)
{
	CoreGraphics::WaitEvent(id, queue);
}

//------------------------------------------------------------------------------
/**
*/
void
EventReset(const EventId id, const CoreGraphicsQueueType queue)
{
	CoreGraphics::ResetEvent(id, queue);
}

//------------------------------------------------------------------------------
/**
*/
void
EventWaitAndReset(const EventId id, const CoreGraphicsQueueType queue)
{
	CoreGraphics::WaitEvent(id, queue);
	CoreGraphics::ResetEvent(id, queue);
}

} // namespace CoreGraphics

#pragma pop_macro("CreateEvent")