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

//------------------------------------------------------------------------------
/**
*/
const VkEventInfo&
EventGetVk(const CoreGraphics::EventId id)
{
    return eventAllocator.Get<Event_Info>(id.id24);
}
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
    VkEventInfo& vkInfo = eventAllocator.Get<Event_Info>(id);

    VkDevice dev = Vulkan::GetCurrentDevice();
    vkCreateEvent(dev, &createInfo, nullptr, &vkInfo.event);
    if (info.createSignaled)
        vkSetEvent(dev, vkInfo.event);

    vkInfo.name = info.name;
    vkInfo.numImageBarriers = 0;
    vkInfo.numBufferBarriers = 0;
    vkInfo.numMemoryBarriers = 0;
    eventAllocator.Get<Event_Device>(id) = dev;

    n_assert(info.textures.Size() < EventMaxNumBarriers);
    n_assert(info.buffers.Size() < EventMaxNumBarriers);

    for (IndexT i = 0; i < info.textures.Size(); i++)
    {
        vkInfo.imageBarriers[vkInfo.numImageBarriers].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        vkInfo.imageBarriers[vkInfo.numImageBarriers].pNext = nullptr;

        vkInfo.imageBarriers[vkInfo.numImageBarriers].srcAccessMask = VkTypes::AsVkAccessFlags(info.fromStage);
        vkInfo.imageBarriers[vkInfo.numImageBarriers].dstAccessMask = VkTypes::AsVkAccessFlags(info.toStage);

        const ImageSubresourceInfo& subres = info.textures[i].subres;
        vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.aspect);
        vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.baseMipLevel = subres.mip;
        vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.levelCount = subres.mipCount;
        vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.baseArrayLayer = subres.layer;
        vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.layerCount = subres.layerCount;
        vkInfo.imageBarriers[vkInfo.numImageBarriers].image = TextureGetVkImage(info.textures[i].tex);
        vkInfo.imageBarriers[vkInfo.numImageBarriers].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkInfo.imageBarriers[vkInfo.numImageBarriers].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkInfo.imageBarriers[vkInfo.numImageBarriers].oldLayout = VkTypes::AsVkImageLayout(info.fromStage);
        vkInfo.imageBarriers[vkInfo.numImageBarriers].newLayout = VkTypes::AsVkImageLayout(info.toStage);
        vkInfo.numImageBarriers++;
    }

    for (IndexT i = 0; i < info.buffers.Size(); i++)
    {
        vkInfo.bufferBarriers[i].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        vkInfo.bufferBarriers[i].pNext = nullptr;

        vkInfo.bufferBarriers[i].srcAccessMask = VkTypes::AsVkAccessFlags(info.fromStage);
        vkInfo.bufferBarriers[i].dstAccessMask = VkTypes::AsVkAccessFlags(info.toStage);

        vkInfo.bufferBarriers[i].buffer = BufferGetVk(info.buffers[i].buf);
        vkInfo.bufferBarriers[i].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkInfo.bufferBarriers[i].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        if (info.buffers[i].size == -1)
        {
            vkInfo.bufferBarriers[vkInfo.numBufferBarriers].offset = 0;
            vkInfo.bufferBarriers[vkInfo.numBufferBarriers].size = VK_WHOLE_SIZE;
        }
        else
        {
            vkInfo.bufferBarriers[vkInfo.numBufferBarriers].offset = info.buffers[i].offset;
            vkInfo.bufferBarriers[vkInfo.numBufferBarriers].size = info.buffers[i].size;
        }

        vkInfo.numBufferBarriers++;
    }

    if (info.textures.Size() == 0 && info.buffers.Size() == 0)
    {
        vkInfo.memoryBarriers[0].sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
        vkInfo.memoryBarriers[0].pNext = nullptr;
        vkInfo.memoryBarriers[0].srcAccessMask = VkTypes::AsVkAccessFlags(info.fromStage);
        vkInfo.memoryBarriers[0].dstAccessMask = VkTypes::AsVkAccessFlags(info.toStage);
        vkInfo.numMemoryBarriers = 1;
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
    VkEventInfo& vkInfo = eventAllocator.Get<Event_Info>(id.id24);
    const VkDevice& dev = eventAllocator.Get<Event_Device>(id.id24);
    vkDestroyEvent(dev, vkInfo.event, nullptr);
    eventAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
EventSignal(const EventId id, const CoreGraphics::CmdBufferId buf, const CoreGraphics::PipelineStage stage)
{
    VkEventInfo& info = eventAllocator.Get<Event_Info>(id.id24);
    vkCmdSetEvent(CmdBufferGetVk(buf), info.event, VkTypes::AsVkPipelineStage(stage));
}

//------------------------------------------------------------------------------
/**
*/
void
EventWait(const EventId id, const CoreGraphics::CmdBufferId buf, const CoreGraphics::PipelineStage waitStage, const CoreGraphics::PipelineStage signalStage)
{
    VkEventInfo& info = eventAllocator.Get<Event_Info>(id.id24);
    vkCmdWaitEvents(CmdBufferGetVk(buf), 1, &info.event,
        VkTypes::AsVkPipelineStage(waitStage),
        VkTypes::AsVkPipelineStage(signalStage),
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
EventReset(const EventId id, const CoreGraphics::CmdBufferId buf, const CoreGraphics::PipelineStage stage)
{
    VkEventInfo& info = eventAllocator.Get<Event_Info>(id.id24);
    vkCmdResetEvent(CmdBufferGetVk(buf), info.event, VkTypes::AsVkPipelineStage(stage));
}

//------------------------------------------------------------------------------
/**
*/
void
EventWaitAndReset(const EventId id, const CoreGraphics::CmdBufferId buf, const CoreGraphics::PipelineStage waitStage, const CoreGraphics::PipelineStage signalStage)
{
    VkEventInfo& info = eventAllocator.Get<Event_Info>(id.id24);
    vkCmdWaitEvents(CmdBufferGetVk(buf), 1, &info.event,
        VkTypes::AsVkPipelineStage(waitStage),
        VkTypes::AsVkPipelineStage(signalStage),
        info.numMemoryBarriers,
        info.memoryBarriers,
        info.numBufferBarriers,
        info.bufferBarriers,
        info.numImageBarriers,
        info.imageBarriers);
    vkCmdResetEvent(CmdBufferGetVk(buf), info.event, VkTypes::AsVkPipelineStage(waitStage));
}

//------------------------------------------------------------------------------
/**
*/
bool 
EventPoll(const EventId id)
{
    const VkEventInfo& info = eventAllocator.Get<Event_Info>(id.id24);
    VkDevice dev = eventAllocator.Get<Event_Device>(id.id24);
    VkResult res = vkGetEventStatus(dev, info.event);
    return res == VK_EVENT_SET;
}

//------------------------------------------------------------------------------
/**
*/
void 
EventHostReset(const EventId id)
{
    const VkEventInfo& info = eventAllocator.Get<Event_Info>(id.id24);
    VkDevice dev = eventAllocator.Get<Event_Device>(id.id24);
    vkResetEvent(dev, info.event);
}

//------------------------------------------------------------------------------
/**
*/
void 
EventHostSignal(const EventId id)
{
    const VkEventInfo& info = eventAllocator.Get<Event_Info>(id.id24);
    VkDevice dev = eventAllocator.Get<Event_Device>(id.id24);
    vkSetEvent(dev, info.event);
}

//------------------------------------------------------------------------------
/**
*/
void
EventHostWait(const EventId id)
{
    const VkEventInfo& info = eventAllocator.Get<Event_Info>(id.id24);
    VkDevice dev = eventAllocator.Get<Event_Device>(id.id24);
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
