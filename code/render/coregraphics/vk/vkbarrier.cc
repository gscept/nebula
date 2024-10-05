//------------------------------------------------------------------------------
// vkbarrier.cc
// (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------

#include "vkbarrier.h"
#include "coregraphics/config.h"
#include "vktypes.h"
#include "vktexture.h"
#include "vkbuffer.h"

#if NEBULA_GRAPHICS_DEBUG
    #define NEBULA_BARRIER_INSERT_MARKER 0 // enable or disable to remove barrier markers
#else
    #define NEBULA_BARRIER_INSERT_MARKER 0
#endif
namespace Vulkan
{
VkBarrierAllocator barrierAllocator(0x00FFFFFF);

//------------------------------------------------------------------------------
/**
*/
const VkBarrierInfo&
BarrierGetVk(const CoreGraphics::BarrierId id)
{
    return barrierAllocator.Get<Barrier_Info>(id.id);
}
}

namespace CoreGraphics
{
using namespace Vulkan;

//------------------------------------------------------------------------------
/**
*/
BarrierId
CreateBarrier(const BarrierCreateInfo& info)
{
    Ids::Id32 id = barrierAllocator.Alloc();
    VkBarrierInfo& vkInfo = barrierAllocator.Get<Barrier_Info>(id);
    Util::Array<CoreGraphics::TextureId>& rts = barrierAllocator.Get<Barrier_Textures>(id);

    rts.Clear();
    vkInfo.name = info.name;
    vkInfo.numImageBarriers = 0;
    vkInfo.numBufferBarriers = 0;
    vkInfo.numMemoryBarriers = 0;
    vkInfo.srcFlags = VkTypes::AsVkPipelineStage(info.fromStage);
    vkInfo.dstFlags = VkTypes::AsVkPipelineStage(info.toStage);
    vkInfo.dep = 0;
    //if (info.domain == BarrierDomain::Pass)
        //vkInfo.dep = VK_DEPENDENCY_BY_REGION_BIT;

    n_assert(info.textures.Size() < MaxNumBarriers);
    n_assert(info.buffers.Size() < MaxNumBarriers);

    for (IndexT i = 0; i < info.textures.Size(); i++)
    {
        vkInfo.imageBarriers[vkInfo.numImageBarriers].sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        vkInfo.imageBarriers[vkInfo.numImageBarriers].pNext = nullptr;

        vkInfo.imageBarriers[vkInfo.numImageBarriers].srcAccessMask = VkTypes::AsVkAccessFlags(info.fromStage);
        vkInfo.imageBarriers[vkInfo.numImageBarriers].dstAccessMask = VkTypes::AsVkAccessFlags(info.toStage);

        const TextureSubresourceInfo& subres = info.textures[i].subres;
        n_assert(subres.bits != ImageBits::Auto);
        bool isDepthStencil = AnyBits(subres.bits, CoreGraphics::ImageBits::DepthBits | CoreGraphics::ImageBits::StencilBits);
        vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.bits);
        vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.baseMipLevel = subres.mip;
        if (subres.mipCount == NEBULA_ALL_MIPS)
            vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
        else
            vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.levelCount = subres.mipCount;
        vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.baseArrayLayer = subres.layer;
        if (subres.layerCount == NEBULA_ALL_LAYERS)
            vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
        else
            vkInfo.imageBarriers[vkInfo.numImageBarriers].subresourceRange.layerCount = subres.layerCount;
        vkInfo.imageBarriers[vkInfo.numImageBarriers].image = TextureGetVkImage(info.textures[i].tex);
        if (info.fromQueue == QueueType::InvalidQueueType)
            vkInfo.imageBarriers[vkInfo.numImageBarriers].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        else
            vkInfo.imageBarriers[vkInfo.numImageBarriers].srcQueueFamilyIndex = CoreGraphics::GetQueueIndex(info.fromQueue);

        if (info.toQueue == QueueType::InvalidQueueType)
            vkInfo.imageBarriers[vkInfo.numImageBarriers].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        else
            vkInfo.imageBarriers[vkInfo.numImageBarriers].dstQueueFamilyIndex = CoreGraphics::GetQueueIndex(info.toQueue);
        vkInfo.imageBarriers[vkInfo.numImageBarriers].oldLayout = VkTypes::AsVkImageLayout(info.fromStage, isDepthStencil);
        vkInfo.imageBarriers[vkInfo.numImageBarriers].newLayout = VkTypes::AsVkImageLayout(info.toStage, isDepthStencil);
        vkInfo.numImageBarriers++;

        rts.Append(info.textures[i].tex);
    }

    for (IndexT i = 0; i < info.buffers.Size(); i++)
    {
        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].pNext = nullptr;

        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].srcAccessMask = VkTypes::AsVkAccessFlags(info.fromStage);
        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].dstAccessMask = VkTypes::AsVkAccessFlags(info.toStage);

        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].buffer = BufferGetVk(info.buffers[i].buf);
        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].offset = 0;
        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].size = VK_WHOLE_SIZE; 

        if (info.buffers[i].subres.size == NEBULA_WHOLE_BUFFER_SIZE)
        {
            vkInfo.bufferBarriers[vkInfo.numBufferBarriers].offset = 0;
            vkInfo.bufferBarriers[vkInfo.numBufferBarriers].size = VK_WHOLE_SIZE;
        }
        else
        {
            vkInfo.bufferBarriers[vkInfo.numBufferBarriers].offset = info.buffers[i].subres.offset;
            vkInfo.bufferBarriers[vkInfo.numBufferBarriers].size = info.buffers[i].subres.size;
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

    BarrierId eventId = id;
    return eventId;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyBarrier(const BarrierId id)
{
    barrierAllocator.Dealloc(id.id);
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierReset(const BarrierId id)
{
    VkBarrierInfo& vkInfo = barrierAllocator.Get<Barrier_Info>(id.id);
    Util::Array<CoreGraphics::TextureId>& rts = barrierAllocator.Get<Barrier_Textures>(id.id);

    IndexT i;
    for (i = 0; i < rts.Size(); i++)
    {
        vkInfo.imageBarriers[i].image = TextureGetVkImage(rts[i]);
    }
}

struct BarrierStackEntry
{
    CoreGraphics::PipelineStage fromStage;
    CoreGraphics::PipelineStage toStage;
    CoreGraphics::BarrierDomain domain;
    Util::FixedArray<TextureBarrierInfo> textures;
    Util::FixedArray<BufferBarrierInfo> buffers;
};

static Util::Stack<BarrierStackEntry> BarrierStack;

//------------------------------------------------------------------------------
/**
*/
void
BarrierPush(const CoreGraphics::CmdBufferId buf
    , CoreGraphics::PipelineStage fromStage
    , CoreGraphics::PipelineStage toStage
    , CoreGraphics::BarrierDomain domain
    , const Util::FixedArray<TextureBarrierInfo, true>& textures
    , const Util::FixedArray<BufferBarrierInfo, true>& buffers)
{
    // first insert the barrier as is
    CmdBarrier(buf, fromStage, toStage, domain, textures, buffers, nullptr);

    // create a stack entry to reverse this barrier
    BarrierStackEntry entry;
    entry.fromStage = fromStage;
    entry.toStage = toStage;
    entry.domain = domain;
    entry.textures = textures;
    entry.buffers = buffers;

    // push to stack
    BarrierStack.Push(entry);
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierPush(const CoreGraphics::CmdBufferId buf
    , CoreGraphics::PipelineStage fromStage
    , CoreGraphics::PipelineStage toStage
    , CoreGraphics::BarrierDomain domain
    , const Util::FixedArray<TextureBarrierInfo, true>& textures)
{
    // first insert the barrier as is
    CmdBarrier(buf, fromStage, toStage, domain, textures, nullptr, nullptr);

    // create a stack entry to reverse this barrier
    BarrierStackEntry entry;
    entry.fromStage = fromStage;
    entry.toStage = toStage;
    entry.domain = domain;
    entry.textures = textures;
    entry.buffers = nullptr;

    // push to stack
    BarrierStack.Push(entry);
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierPush(const CoreGraphics::CmdBufferId buf
    , CoreGraphics::PipelineStage fromStage
    , CoreGraphics::PipelineStage toStage
    , CoreGraphics::BarrierDomain domain
    , const Util::FixedArray<BufferBarrierInfo, true>& buffers)
{
    // first insert the barrier as is
    CmdBarrier(buf, fromStage, toStage, domain, nullptr, buffers, nullptr);

    // create a stack entry to reverse this barrier
    BarrierStackEntry entry;
    entry.fromStage = fromStage;
    entry.toStage = toStage;
    entry.domain = domain;
    entry.textures = nullptr;
    entry.buffers = buffers;

    // push to stack
    BarrierStack.Push(entry);
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierPop(const CoreGraphics::CmdBufferId buf)
{
    // pop from stack
    BarrierStackEntry entry = BarrierStack.Pop();

    // insert barrier
    CmdBarrier(buf, entry.toStage, entry.fromStage, entry.domain, entry.textures, entry.buffers, nullptr);
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierRepeat(const CoreGraphics::CmdBufferId buf)
{
    // pop from stack
    const BarrierStackEntry& entry = BarrierStack.Peek();

    // insert barrier
    CmdBarrier(buf, entry.fromStage, entry.toStage, entry.domain, entry.textures, entry.buffers, nullptr);
}

} // namespace CoreGraphics
