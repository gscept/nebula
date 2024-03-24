#pragma once
//------------------------------------------------------------------------------
/**
    The Vulkan implementation of a GPU command barrier

    @copyright
    (C) 2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "coregraphics/barrier.h"

namespace Vulkan
{

static const SizeT MaxNumBarriers = 16;

struct VkBarrierInfo
{
    Util::StringAtom name;
    VkPipelineStageFlags srcFlags;
    VkPipelineStageFlags dstFlags;
    VkDependencyFlags dep;
    uint32_t numMemoryBarriers;
    VkMemoryBarrier memoryBarriers[MaxNumBarriers];
    uint32_t numBufferBarriers;
    VkBufferMemoryBarrier bufferBarriers[MaxNumBarriers];
    uint32_t numImageBarriers;
    VkImageMemoryBarrier imageBarriers[MaxNumBarriers];
};

enum
{
    Barrier_Info
    , Barrier_Textures  // This is for reloading textures
    , Barrier_Buffers
};

typedef Ids::IdAllocator<
    VkBarrierInfo
    , Util::Array<CoreGraphics::TextureId>
> VkBarrierAllocator;
extern VkBarrierAllocator barrierAllocator;

/// Get Vulkan info
const VkBarrierInfo& BarrierGetVk(const CoreGraphics::BarrierId id);


} // namespace Vulkan
