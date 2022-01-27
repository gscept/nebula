//------------------------------------------------------------------------------
// vkbarrier.cc
// (C) 2017-2020 Individual contributors, see AUTHORS file
//------------------------------------------------------------------------------
#include "render/stdneb.h"
#include "vkbarrier.h"
#include "coregraphics/config.h"
#include "vkcommandbuffer.h"
#include "vktypes.h"
#include "vktexture.h"
#include "vkbuffer.h"
#include "coregraphics/vk/vkgraphicsdevice.h"

#if NEBULA_GRAPHICS_DEBUG
    #define NEBULA_BARRIER_INSERT_MARKER 0 // enable or disable to remove barrier markers
#else
    #define NEBULA_BARRIER_INSERT_MARKER 0
#endif
namespace Vulkan
{
VkBarrierAllocator barrierAllocator(0x00FFFFFF);
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
    VkBarrierInfo& vkInfo = barrierAllocator.Get<0>(id);
    Util::Array<CoreGraphics::TextureId>& rts = barrierAllocator.Get<1>(id);

    rts.Clear();
    vkInfo.name = info.name;
    vkInfo.numImageBarriers = 0;
    vkInfo.numBufferBarriers = 0;
    vkInfo.numMemoryBarriers = 0;
    vkInfo.srcFlags = VkTypes::AsVkPipelineFlags(info.leftDependency);
    vkInfo.dstFlags = VkTypes::AsVkPipelineFlags(info.rightDependency);

    if (info.domain == BarrierDomain::Pass)
        vkInfo.dep = VK_DEPENDENCY_BY_REGION_BIT;

    n_assert(info.textures.Size() < MaxNumBarriers);
    n_assert(info.rwBuffers.Size() < MaxNumBarriers);
    n_assert(info.barriers.Size() < MaxNumBarriers);

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

        rts.Append(info.textures[i].tex);
    }

    for (IndexT i = 0; i < info.rwBuffers.Size(); i++)
    {
        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].pNext = nullptr;

        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].srcAccessMask = VkTypes::AsVkResourceAccessFlags(info.rwBuffers[i].fromAccess);
        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].dstAccessMask = VkTypes::AsVkResourceAccessFlags(info.rwBuffers[i].toAccess);

        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].buffer = BufferGetVk(info.rwBuffers[i].buf);
        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].offset = 0;
        vkInfo.bufferBarriers[vkInfo.numBufferBarriers].size = VK_WHOLE_SIZE; 

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


    BarrierId eventId;
    eventId.id24 = id;
    eventId.id8 = BarrierIdType;
    return eventId;
}

//------------------------------------------------------------------------------
/**
*/
void
DestroyBarrier(const BarrierId id)
{
    barrierAllocator.Dealloc(id.id24);
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierInsert(const BarrierId id, const CoreGraphics::QueueType queue)
{
#if NEBULA_BARRIER_INSERT_MARKER
    const Util::StringAtom& name = barrierAllocator.Get<0>(id.id24).name;
    CommandBufferBeginMarker(queue, NEBULA_MARKER_GRAY, name.Value());
#endif
    CoreGraphics::InsertBarrier(id, queue);

#if NEBULA_BARRIER_INSERT_MARKER
    CommandBufferEndMarker(queue);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierReset(const BarrierId id)
{
    VkBarrierInfo& vkInfo = barrierAllocator.Get<0>(id.id24);
    Util::Array<CoreGraphics::TextureId>& rts = barrierAllocator.Get<1>(id.id24);

    IndexT i;
    for (i = 0; i < rts.Size(); i++)
    {
        vkInfo.imageBarriers[i].image = TextureGetVkImage(rts[i]);
    }
}

//------------------------------------------------------------------------------
/**
    Validate barriers to make sure they are compatible, and don't rely on the validation layers
*/
void
ValidateBarrier(
    BarrierStage stage
    , BarrierAccess access)
{
#if NEBULA_GRAPHICS_DEBUG
    switch (access)
    {
    case BarrierAccess::IndirectRead:
        n_assert(stage == BarrierStage::Indirect);
        break;
    case BarrierAccess::IndexRead:
    case BarrierAccess::VertexRead:
        n_assert(stage == BarrierStage::VertexInput);
        break;
    case BarrierAccess::UniformRead:
    case BarrierAccess::ShaderRead:
    case BarrierAccess::ShaderWrite:
        n_assert(
            AllBits(stage, BarrierStage::VertexShader)
            || AllBits(stage, BarrierStage::HullShader)
            || AllBits(stage, BarrierStage::DomainShader)
            || AllBits(stage, BarrierStage::GeometryShader)
            || AllBits(stage, BarrierStage::PixelShader)
            || AllBits(stage, BarrierStage::ComputeShader)
        );
        break;
    case BarrierAccess::InputAttachmentRead:
        n_assert(stage == BarrierStage::PixelShader);
        break;
    case BarrierAccess::ColorAttachmentRead:
    case BarrierAccess::ColorAttachmentWrite:
        n_assert(stage == BarrierStage::PassOutput);
        break;
    case BarrierAccess::DepthAttachmentRead:
    case BarrierAccess::DepthAttachmentWrite:
        n_assert(
            AllBits(stage, BarrierStage::EarlyDepth)
            || AllBits(stage, BarrierStage::LateDepth)
        );
        break;
    case BarrierAccess::TransferRead:
    case BarrierAccess::TransferWrite:
        n_assert(stage == BarrierStage::Transfer);
        break;
    case BarrierAccess::HostRead:
    case BarrierAccess::HostWrite:
        n_assert(stage == BarrierStage::Host);
        break;
    case BarrierAccess::MemoryRead:
    case BarrierAccess::MemoryWrite:

        // do nothing for these
        break;
    }
#endif
}

//------------------------------------------------------------------------------
/**
*/
void 
BarrierInsert(
    const QueueType queue, 
    BarrierStage fromStage,
    BarrierStage toStage,
    BarrierDomain domain,
    const Util::FixedArray<TextureBarrier>& textures,
    const Util::FixedArray<BufferBarrier>& rwBuffers,
    const char* name)
{
    VkBarrierInfo barrier;
    barrier.name = name;
    barrier.srcFlags = VkTypes::AsVkPipelineFlags(fromStage);
    barrier.dstFlags = VkTypes::AsVkPipelineFlags(toStage);
    barrier.dep = domain == BarrierDomain::Pass ? VK_DEPENDENCY_BY_REGION_BIT : 0;
    barrier.numBufferBarriers = rwBuffers.Size();
    for (uint32_t i = 0; i < barrier.numBufferBarriers; i++)
    {
        VkBufferMemoryBarrier& vkBar = barrier.bufferBarriers[i];
        BufferBarrier& nebBar = rwBuffers[i];

        ValidateBarrier(fromStage, nebBar.fromAccess);
        ValidateBarrier(toStage, nebBar.toAccess);
        vkBar.srcAccessMask = VkTypes::AsVkResourceAccessFlags(nebBar.fromAccess);
        vkBar.dstAccessMask = VkTypes::AsVkResourceAccessFlags(nebBar.toAccess);

        vkBar.buffer = CoreGraphics::BufferGetVk(nebBar.buf);
        vkBar.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        vkBar.pNext = nullptr;
        vkBar.offset = nebBar.offset;
        vkBar.size = (nebBar.size == -1) ? VK_WHOLE_SIZE : nebBar.size;
        vkBar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkBar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkBar.pNext = nullptr;
        vkBar.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    }
    barrier.numImageBarriers = textures.Size();
    IndexT i, j = 0;
    for (i = 0; i < textures.Size(); i++, j++)
    {
        VkImageMemoryBarrier& vkBar = barrier.imageBarriers[j];
        TextureBarrier& nebBar = textures[i];

        ValidateBarrier(fromStage, nebBar.fromAccess);
        ValidateBarrier(toStage, nebBar.toAccess);
        vkBar.srcAccessMask = VkTypes::AsVkResourceAccessFlags(nebBar.fromAccess);
        vkBar.dstAccessMask = VkTypes::AsVkResourceAccessFlags(nebBar.toAccess);

        const ImageSubresourceInfo& subres = nebBar.subres;
        vkBar.subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.aspect);
        vkBar.subresourceRange.baseMipLevel = subres.mip;
        vkBar.subresourceRange.levelCount = subres.mipCount;
        vkBar.subresourceRange.baseArrayLayer = subres.layer;
        vkBar.subresourceRange.layerCount = subres.layerCount;
        vkBar.image = CoreGraphics::TextureGetVkImage(nebBar.tex);
        vkBar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkBar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkBar.oldLayout = VkTypes::AsVkImageLayout(nebBar.fromLayout);
        vkBar.newLayout = VkTypes::AsVkImageLayout(nebBar.toLayout);
        vkBar.pNext = nullptr;
        vkBar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    }
    barrier.numMemoryBarriers = 0; // maybe support this?

#if NEBULA_BARRIER_INSERT_MARKER
    CommandBufferBeginMarker(queue, NEBULA_MARKER_GRAY, name);
#endif
    Vulkan::InsertBarrier(barrier, queue);

#if NEBULA_BARRIER_INSERT_MARKER
    CommandBufferEndMarker(queue);
#endif
}

//------------------------------------------------------------------------------
/**
*/
void 
BarrierInsert(
    const CoreGraphics::CommandBufferId buf,
    CoreGraphics::BarrierStage fromStage,
    CoreGraphics::BarrierStage toStage,
    CoreGraphics::BarrierDomain domain,
    const Util::FixedArray<TextureBarrier>& textures,
    const Util::FixedArray<BufferBarrier>& rwBuffers,
    const char* name)
{
    VkBarrierInfo barrier;
    barrier.name = name;
    barrier.srcFlags = VkTypes::AsVkPipelineFlags(fromStage);
    barrier.dstFlags = VkTypes::AsVkPipelineFlags(toStage);
    barrier.dep = domain == CoreGraphics::BarrierDomain::Pass ? VK_DEPENDENCY_BY_REGION_BIT : 0;
    barrier.numBufferBarriers = rwBuffers.Size();
    for (uint32_t i = 0; i < barrier.numBufferBarriers; i++)
    {
        VkBufferMemoryBarrier& vkBar = barrier.bufferBarriers[i];
        BufferBarrier& nebBar = rwBuffers[i];

        ValidateBarrier(fromStage, nebBar.fromAccess);
        ValidateBarrier(toStage, nebBar.toAccess);
        vkBar.srcAccessMask = VkTypes::AsVkResourceAccessFlags(nebBar.fromAccess);
        vkBar.dstAccessMask = VkTypes::AsVkResourceAccessFlags(nebBar.toAccess);

        vkBar.buffer = CoreGraphics::BufferGetVk(nebBar.buf);
        vkBar.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
        vkBar.pNext = nullptr;
        vkBar.offset = nebBar.offset;
        vkBar.size = (nebBar.size == -1) ? VK_WHOLE_SIZE : nebBar.size;
        vkBar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkBar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkBar.pNext = nullptr;
        vkBar.sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
    }
    barrier.numImageBarriers = textures.Size();
    IndexT i, j = 0;
    for (i = 0; i < textures.Size(); i++, j++)
    {
        VkImageMemoryBarrier& vkBar = barrier.imageBarriers[j];
        TextureBarrier& nebBar = textures[i];

        ValidateBarrier(fromStage, nebBar.fromAccess);
        ValidateBarrier(toStage, nebBar.toAccess);
        vkBar.srcAccessMask = VkTypes::AsVkResourceAccessFlags(nebBar.fromAccess);
        vkBar.dstAccessMask = VkTypes::AsVkResourceAccessFlags(nebBar.toAccess);

        const ImageSubresourceInfo& subres = nebBar.subres;
        vkBar.subresourceRange.aspectMask = VkTypes::AsVkImageAspectFlags(subres.aspect);
        vkBar.subresourceRange.baseMipLevel = subres.mip;
        vkBar.subresourceRange.levelCount = subres.mipCount;
        vkBar.subresourceRange.baseArrayLayer = subres.layer;
        vkBar.subresourceRange.layerCount = subres.layerCount;
        vkBar.image = CoreGraphics::TextureGetVkImage(nebBar.tex);
        vkBar.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkBar.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        vkBar.oldLayout = VkTypes::AsVkImageLayout(nebBar.fromLayout);
        vkBar.newLayout = VkTypes::AsVkImageLayout(nebBar.toLayout);

        vkBar.pNext = nullptr;
        vkBar.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    }
    barrier.numMemoryBarriers = 0; // maybe support this?

    // insert barrier
    vkCmdPipelineBarrier(CommandBufferGetVk(buf),
        barrier.srcFlags,
        barrier.dstFlags,
        barrier.dep,
        barrier.numMemoryBarriers, barrier.memoryBarriers,
        barrier.numBufferBarriers, barrier.bufferBarriers,
        barrier.numImageBarriers, barrier.imageBarriers);
}

//------------------------------------------------------------------------------
/**
*/
void 
BarrierInsert(
    const CoreGraphics::QueueType queue, 
    CoreGraphics::BarrierStage fromStage,
    CoreGraphics::BarrierStage toStage,
    CoreGraphics::BarrierDomain domain,
    const Util::FixedArray<ExecutionBarrier>& barriers,
    const char* name)
{
    VkBarrierInfo barrier;
    barrier.name = name;
    barrier.srcFlags = VkTypes::AsVkPipelineFlags(fromStage);
    barrier.dstFlags = VkTypes::AsVkPipelineFlags(toStage);
    barrier.dep = domain == CoreGraphics::BarrierDomain::Pass ? VK_DEPENDENCY_BY_REGION_BIT : 0;
    barrier.numImageBarriers = 0;
    barrier.numBufferBarriers = 0;
    barrier.numMemoryBarriers = barriers.Size();
    for (int i = 0; i < barriers.Size(); i++)
    {
        ValidateBarrier(fromStage, barriers[i].fromAccess);
        ValidateBarrier(toStage, barriers[i].toAccess);
        VkMemoryBarrier memBarrier =
        {
            VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            nullptr,
            VkTypes::AsVkResourceAccessFlags(barriers[i].fromAccess),
            VkTypes::AsVkResourceAccessFlags(barriers[i].toAccess)
        };
        barrier.memoryBarriers[i] = memBarrier;
    }

#if NEBULA_BARRIER_INSERT_MARKER
    CommandBufferBeginMarker(queue, NEBULA_MARKER_GRAY, name);
#endif

    Vulkan::InsertBarrier(barrier, queue);

#if NEBULA_BARRIER_INSERT_MARKER
    CommandBufferEndMarker(queue);
#endif

}

//------------------------------------------------------------------------------
/**
*/
void 
BarrierInsert(
    const CoreGraphics::CommandBufferId buf, 
    CoreGraphics::BarrierStage fromStage,
    CoreGraphics::BarrierStage toStage,
    CoreGraphics::BarrierDomain domain,
    const Util::FixedArray<ExecutionBarrier>& barriers,
    const char* name)
{
    VkBarrierInfo barrier;
    barrier.name = name;
    barrier.srcFlags = VkTypes::AsVkPipelineFlags(fromStage);
    barrier.dstFlags = VkTypes::AsVkPipelineFlags(toStage);
    barrier.dep = domain == CoreGraphics::BarrierDomain::Pass ? VK_DEPENDENCY_BY_REGION_BIT : 0;
    barrier.numImageBarriers = 0;
    barrier.numBufferBarriers = 0;
    barrier.numMemoryBarriers = barriers.Size();
    for (int i = 0; i < barriers.Size(); i++)
    {
        ValidateBarrier(fromStage, barriers[i].fromAccess);
        ValidateBarrier(toStage, barriers[i].toAccess);
        VkMemoryBarrier memBarrier =
        {
            VK_STRUCTURE_TYPE_MEMORY_BARRIER,
            nullptr,
            VkTypes::AsVkResourceAccessFlags(barriers[i].fromAccess),
            VkTypes::AsVkResourceAccessFlags(barriers[i].toAccess)
        };
        barrier.memoryBarriers[i] = memBarrier;
    }

    // insert barrier
    vkCmdPipelineBarrier(CommandBufferGetVk(buf),
        barrier.srcFlags,
        barrier.dstFlags,
        barrier.dep,
        barrier.numMemoryBarriers, barrier.memoryBarriers,
        barrier.numBufferBarriers, barrier.bufferBarriers,
        barrier.numImageBarriers, barrier.imageBarriers);
}

struct BarrierStackEntry
{
    CoreGraphics::BarrierStage fromStage;
    CoreGraphics::BarrierStage toStage;
    CoreGraphics::BarrierDomain domain;
    Util::FixedArray<TextureBarrier> textures;
    Util::FixedArray<BufferBarrier> buffers;
};

static Util::Stack<BarrierStackEntry> BarrierStack;

//------------------------------------------------------------------------------
/**
*/
void
BarrierPush(const CoreGraphics::QueueType queue
    , CoreGraphics::BarrierStage fromStage
    , CoreGraphics::BarrierStage toStage
    , CoreGraphics::BarrierDomain domain
    , const Util::FixedArray<TextureBarrier>& textures
    , const Util::FixedArray<BufferBarrier>& buffers)
{
    // first insert the barrier as is
    BarrierInsert(queue, fromStage, toStage, domain, textures, buffers);

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
BarrierPush(const CoreGraphics::QueueType queue
    , CoreGraphics::BarrierStage fromStage
    , CoreGraphics::BarrierStage toStage
    , CoreGraphics::BarrierDomain domain
    , const Util::FixedArray<TextureBarrier>& textures)
{
    // first insert the barrier as is
    BarrierInsert(queue, fromStage, toStage, domain, textures, nullptr);

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
BarrierPush(const CoreGraphics::QueueType queue
    , CoreGraphics::BarrierStage fromStage
    , CoreGraphics::BarrierStage toStage
    , CoreGraphics::BarrierDomain domain
    , const Util::FixedArray<BufferBarrier>& buffers)
{
    // first insert the barrier as is
    BarrierInsert(queue, fromStage, toStage, domain, nullptr, buffers);

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
BarrierPop(const CoreGraphics::QueueType queue)
{
    // pop from stack
    BarrierStackEntry entry = BarrierStack.Pop();

    // reverse barriers
    for (TextureBarrier& texture : entry.textures)
    {
        // flip dependencies
        auto intermediateAccess = texture.toAccess;
        auto intermediateLayout = texture.toLayout;
        texture.toAccess = texture.fromAccess;
        texture.toLayout = texture.fromLayout;
        texture.fromAccess = intermediateAccess;
        texture.fromLayout = intermediateLayout;
    }
    for (BufferBarrier& buffer : entry.buffers)
    {
        auto intermediateAccess = buffer.toAccess;
        buffer.toAccess = buffer.fromAccess;
        buffer.fromAccess = intermediateAccess;
    }

    // insert barrier
    BarrierInsert(queue, entry.toStage, entry.fromStage, entry.domain, entry.textures, entry.buffers);
}

//------------------------------------------------------------------------------
/**
*/
void
BarrierRepeat(const CoreGraphics::QueueType queue)
{
    // pop from stack
    const BarrierStackEntry& entry = BarrierStack.Peek();

    // insert barrier
    BarrierInsert(queue, entry.fromStage, entry.toStage, entry.domain, entry.textures, entry.buffers);
}

} // namespace CoreGraphics
