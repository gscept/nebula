#pragma once
//------------------------------------------------------------------------------
/**
    Implements a Vulkan command buffer

    (C)2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "vkloader.h"
#include "coregraphics/commandbuffer.h"

namespace Vulkan
{

/// get vk command buffer pool
const VkCommandPool CommandBufferPoolGetVk(const CoreGraphics::CommandBufferPoolId id);
/// get vk device that created the pool
const VkDevice CommandBufferPoolGetVkDevice(const CoreGraphics::CommandBufferPoolId id);

enum
{
    CommandBufferPool_VkDevice,
    CommandBufferPool_VkCommandPool,
};
typedef Ids::IdAllocator<VkDevice, VkCommandPool> VkCommandBufferPoolAllocator;

//------------------------------------------------------------------------------
/**
*/
static const uint NumPoolTypes = 4;
struct CommandBufferPools
{
    VkCommandPool pools[CoreGraphics::NumCommandBufferUsages][NumPoolTypes];
    uint queueFamilies[CoreGraphics::NumCommandBufferUsages];
    VkDevice dev;
};

/// get vk command buffer
const VkCommandBuffer CommandBufferGetVk(const CoreGraphics::CommandBufferId id);

enum
{
    CommandBuffer_VkDevice,
    CommandBuffer_VkCommandBuffer,
    CommandBuffer_VkCommandPool
};
typedef Ids::IdAllocatorSafe<VkDevice, VkCommandBuffer, VkCommandPool> VkCommandBufferAllocator;

} // Vulkan
