//------------------------------------------------------------------------------
//  vkevent.cc
//  (C)2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkevent.h"
#include "coregraphics/event.h"
#include "vkgraphicsdevice.h"
#include "vkcommandbuffer.h"
#include "coregraphics/config.h"
#include "vktypes.h"
#include "vktexture.h"
#include "vkbuffer.h"

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
    if (info.createSignaled)
        vkSetEvent(dev, vkInfo.event);

    vkInfo.name = info.name;
    vkInfo.numImageBarriers = 0;
    vkInfo.numBufferBarriers = 0;
    vkInfo.numMemoryBarriers = 0;
    eventAllocator.Get<0>(id) = dev;

    n_assert(info.textures.Size() < EventMaxNumBarriers);
    n_assert(info.rwBuffers.Size() < EventMaxNumBarriers);
    n_assert(info.barriers.Size() < EventMaxNumBarriers);

    for (IndexT i = 0; i < info.textures.Size(); i++)
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

    for (IndexT i = 0; i < info.rwBuffers.Size(); i++)
    {
        vkInfo.bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        vkInfo.bufferBarriers[i].pNext = nullptr;

        vkInfo.bufferBarriers[i].srcAccessMask = VkTypes::AsVkResourceAccessFlags(info.rwBuffers[i].fromAccess);
        vkInfo.bufferBarriers[i].dstAccessMask = VkTypes::AsVkResourceAccessFlags(info.rwBuffers[i].toAccess);

        vkInfo.bufferBarriers[i].buffer = BufferGetVk(info.rwBuffers[i].buf);
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
    }

    for (IndexT i = 0; i < info.barriers.Size(); i++)
    {
        vkInfo.memoryBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        vkInfo.memoryBarriers[i].pNext = nullptr;

        vkInfo.memoryBarriers[i].srcAccessMask = VkTypes::AsVkResourceAccessFlags(info.barriers[i].fromAccess);
        vkInfo.memoryBarriers[i].dstAccessMask = VkTypes::AsVkResourceAccessFlags(info.barriers[i].toAccess);

        vkInfo.numMemoryBarriers++;
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
EventSignal(const EventId id, const CoreGraphics::QueueType queue, const CoreGraphics::BarrierStage stage)
{
#if NEBULA_GRAPHICS_DEBUG
    const Util::StringAtom& name = eventAllocator.Get<1>(id.id24).name;
    CommandBufferBeginMarker(queue, NEBULA_MARKER_ORANGE, name.Value());
#endif
    CoreGraphics::SignalEvent(id, stage, queue);

#if NEBULA_GRAPHICS_DEBUG
    CommandBufferEndMarker(queue);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
EventSignal(const EventId id, const CoreGraphics::CommandBufferId buf, const CoreGraphics::BarrierStage stage)
{
    VkEventInfo& info = eventAllocator.Get<1>(id.id24);
    vkCmdSetEvent(CommandBufferGetVk(buf), info.event, VkTypes::AsVkPipelineFlags(stage));
}

//------------------------------------------------------------------------------
/**
*/
void
EventWait(
    const EventId id,
    const CoreGraphics::QueueType queue,
    const CoreGraphics::BarrierStage waitStage,
    const CoreGraphics::BarrierStage signalStage
    )
{
#if NEBULA_GRAPHICS_DEBUG
    const Util::StringAtom& name = eventAllocator.Get<1>(id.id24).name;
    CommandBufferBeginMarker(queue, NEBULA_MARKER_ORANGE, name.Value());
#endif
    CoreGraphics::WaitEvent(id, waitStage, signalStage, queue);

#if NEBULA_GRAPHICS_DEBUG
    CommandBufferEndMarker(queue);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
EventWait(const EventId id, const CoreGraphics::CommandBufferId buf, const CoreGraphics::BarrierStage waitStage, const CoreGraphics::BarrierStage signalStage)
{
    VkEventInfo& info = eventAllocator.Get<1>(id.id24);
    vkCmdWaitEvents(CommandBufferGetVk(buf), 1, &info.event,
        VkTypes::AsVkPipelineFlags(waitStage),
        VkTypes::AsVkPipelineFlags(signalStage),
        info.numMemoryBarriers,
        info.memoryBarriers,
        info.numBufferBarriers,
        info.bufferBarriers,
        info.numImageBarriers,
        info.imageBarriers);
}

//------------------------------------------------------------------------------
/**
*/
void
EventReset(const EventId id, const CoreGraphics::QueueType queue, const CoreGraphics::BarrierStage stage)
{
#if NEBULA_GRAPHICS_DEBUG
    const Util::StringAtom& name = eventAllocator.Get<1>(id.id24).name;
    CommandBufferBeginMarker(queue, NEBULA_MARKER_ORANGE, name.Value());
#endif

    CoreGraphics::ResetEvent(id, stage, queue);

#if NEBULA_GRAPHICS_DEBUG
    CommandBufferEndMarker(queue);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
EventReset(const EventId id, const CoreGraphics::CommandBufferId buf, const CoreGraphics::BarrierStage stage)
{
    VkEventInfo& info = eventAllocator.Get<1>(id.id24);
    vkCmdResetEvent(CommandBufferGetVk(buf), info.event, VkTypes::AsVkPipelineFlags(stage));
}

//------------------------------------------------------------------------------
/**
*/
void
EventWaitAndReset(const EventId id, const CoreGraphics::QueueType queue, const CoreGraphics::BarrierStage waitStage, const CoreGraphics::BarrierStage signalStage)
{
#if NEBULA_GRAPHICS_DEBUG
    const Util::StringAtom& name = eventAllocator.Get<1>(id.id24).name;
    CommandBufferBeginMarker(queue, NEBULA_MARKER_ORANGE, name.Value());
#endif

    CoreGraphics::WaitEvent(id, waitStage, signalStage, queue);
    CoreGraphics::ResetEvent(id, signalStage, queue);

#if NEBULA_GRAPHICS_DEBUG
    CommandBufferEndMarker(queue);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
EventWaitAndReset(const EventId id, const CoreGraphics::CommandBufferId buf, const CoreGraphics::BarrierStage waitStage, const CoreGraphics::BarrierStage signalStage)
{
    VkEventInfo& info = eventAllocator.Get<1>(id.id24);
    vkCmdWaitEvents(CommandBufferGetVk(buf), 1, &info.event,
        VkTypes::AsVkPipelineFlags(waitStage),
        VkTypes::AsVkPipelineFlags(signalStage),
        info.numMemoryBarriers,
        info.memoryBarriers,
        info.numBufferBarriers,
        info.bufferBarriers,
        info.numImageBarriers,
        info.imageBarriers);
    vkCmdResetEvent(CommandBufferGetVk(buf), info.event, VkTypes::AsVkPipelineFlags(waitStage));
}

//------------------------------------------------------------------------------
/**
*/
bool 
EventPoll(const EventId id)
{
    const VkEventInfo& info = eventAllocator.Get<1>(id.id24);
    VkDevice dev = eventAllocator.Get<0>(id.id24);
    VkResult res = vkGetEventStatus(dev, info.event);
    return res == VK_EVENT_SET;
}

//------------------------------------------------------------------------------
/**
*/
void 
EventHostReset(const EventId id)
{
    const VkEventInfo& info = eventAllocator.Get<1>(id.id24);
    VkDevice dev = eventAllocator.Get<0>(id.id24);
    vkResetEvent(dev, info.event);
}

//------------------------------------------------------------------------------
/**
*/
void 
EventHostSignal(const EventId id)
{
    const VkEventInfo& info = eventAllocator.Get<1>(id.id24);
    VkDevice dev = eventAllocator.Get<0>(id.id24);
    vkSetEvent(dev, info.event);
}

//------------------------------------------------------------------------------
/**
*/
void
EventHostWait(const EventId id)
{
    const VkEventInfo& info = eventAllocator.Get<1>(id.id24);
    VkDevice dev = eventAllocator.Get<0>(id.id24);
    VkResult res;
    while (true)
    {
        res = vkGetEventStatus(dev, info.event);
        if (res == VK_EVENT_SET)
            break;
    }

}

} // namespace CoreGraphics

#pragma pop_macro("CreateEvent")
