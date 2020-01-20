//------------------------------------------------------------------------------
//  vkevent.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkevent.h"
#include "coregraphics/event.h"
#include "vkgraphicsdevice.h"
#include "coregraphics/config.h"
#include "vktypes.h"
#include "vktexture.h"
#include "vkshaderrwbuffer.h"

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

	vkInfo.name = info.name;
	vkInfo.numImageBarriers = 0;
	vkInfo.numBufferBarriers = 0;
	vkInfo.numMemoryBarriers = 0;
	vkInfo.leftDependency = VkTypes::AsVkPipelineFlags(info.leftDependency);
	vkInfo.rightDependency = VkTypes::AsVkPipelineFlags(info.rightDependency);
	eventAllocator.Get<0>(id) = dev;

	n_assert(info.textures.Size() < EventMaxNumBarriers);
	n_assert(info.rwBuffers.Size() < EventMaxNumBarriers);

	IndexT i;
	for (i = 0; i < info.textures.Size(); i++)
	{
		vkInfo.imageBarriers[vkInfo.numImageBarriers].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].pNext = nullptr;

		vkInfo.imageBarriers[vkInfo.numImageBarriers].srcAccessMask = VkTypes::AsVkResourceAccessFlags(info.textures[i].fromAccess);
		vkInfo.imageBarriers[vkInfo.numImageBarriers].dstAccessMask = VkTypes::AsVkResourceAccessFlags(info.textures[i].toAccess);

		const ImageSubresourceInfo& subres = info.textures[i].subres;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.aspect);
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.baseMipLevel = subres.mip;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.levelCount = subres.mipCount;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.baseArrayLayer = subres.layer;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.layerCount = subres.layerCount;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].image = TextureGetVkImage(info.textures[i].tex);
		vkInfo.imageBarriers[vkInfo.numImageBarriers].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.imageBarriers[vkInfo.numImageBarriers].oldLayout = VkTypes::AsVkImageLayout(info.textures[i].fromLayout);
		vkInfo.imageBarriers[vkInfo.numImageBarriers].newLayout = VkTypes::AsVkImageLayout(info.textures[i].toLayout);
		vkInfo.numImageBarriers++;
	}

	for (i = 0; i < info.rwBuffers.Size(); i++)
	{
		vkInfo.bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		vkInfo.bufferBarriers[i].pNext = nullptr;

		vkInfo.bufferBarriers[i].srcAccessMask = VkTypes::AsVkResourceAccessFlags(info.rwBuffers[i].fromAccess);
		vkInfo.bufferBarriers[i].dstAccessMask = VkTypes::AsVkResourceAccessFlags(info.rwBuffers[i].toAccess);

		vkInfo.bufferBarriers[i].buffer = ShaderRWBufferGetVkBuffer(info.rwBuffers[i].buf);
		vkInfo.bufferBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		vkInfo.bufferBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

		if (info.rwBuffers[i].size == -1)
		{
			vkInfo.bufferBarriers[vkInfo.numBufferBarriers].offset = 0;
			vkInfo.bufferBarriers[vkInfo.numBufferBarriers].size = VK_WHOLE_SIZE;
		}
		else
		{
			vkInfo.bufferBarriers[vkInfo.numBufferBarriers].offset = info.rwBuffers[i].offset;
			vkInfo.bufferBarriers[vkInfo.numBufferBarriers].size = info.rwBuffers[i].size;
		}

		vkInfo.numBufferBarriers++;

		vkInfo.bufferBarriers[i].pNext = nullptr;
		vkInfo.bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
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
EventSignal(const EventId id, const CoreGraphics::QueueType queue)
{
#if NEBULA_GRAPHICS_DEBUG
	const Util::StringAtom& name = eventAllocator.Get<1>(id.id24).name;
	CommandBufferBeginMarker(queue, NEBULA_MARKER_PURPLE, (name.AsString() + " Signal").AsCharPtr());
#endif
	CoreGraphics::SignalEvent(id, queue);

#if NEBULA_GRAPHICS_DEBUG
	CommandBufferEndMarker(queue);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
EventWait(const EventId id, const CoreGraphics::QueueType queue)
{
#if NEBULA_GRAPHICS_DEBUG
	const Util::StringAtom& name = eventAllocator.Get<1>(id.id24).name;
	CommandBufferBeginMarker(queue, NEBULA_MARKER_ORANGE, (name.AsString() + " Wait").AsCharPtr());
#endif
	CoreGraphics::WaitEvent(id, queue);

#if NEBULA_GRAPHICS_DEBUG
	CommandBufferEndMarker(queue);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
EventReset(const EventId id, const CoreGraphics::QueueType queue)
{
	CoreGraphics::ResetEvent(id, queue);
}

//------------------------------------------------------------------------------
/**
*/
void
EventWaitAndReset(const EventId id, const CoreGraphics::QueueType queue)
{
	CoreGraphics::WaitEvent(id, queue);
	CoreGraphics::ResetEvent(id, queue);
}

} // namespace CoreGraphics

#pragma pop_macro("CreateEvent")