#pragma once
//------------------------------------------------------------------------------
/**
	Implements a Vulkan command buffer

	(C)2017-2020 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "vulkan/vulkan.h"
#include "coregraphics/commandbuffer.h"

namespace Vulkan
{

static const uint NumPoolTypes = 4;
struct CommandBufferPools
{

	VkCommandPool pools[CoreGraphics::NumCommandBufferUsages][NumPoolTypes];
	uint queueFamilies[CoreGraphics::NumCommandBufferUsages];
	VkDevice dev;
};

/// setup pools for a certain device
void SetupVkPools(VkDevice dev, uint32_t drawQueue, uint32_t computeQueue, uint32_t transferQueue, uint32_t sparseQueue);
/// destroy pools
void DestroyVkPools(VkDevice dev);

/// get pool based on flags
const VkCommandPool CommandBufferGetVkPool(CoreGraphics::CommandBufferUsage usage, VkCommandPoolCreateFlags flags);

/// get vk command buffer
const VkCommandBuffer CommandBufferGetVk(const CoreGraphics::CommandBufferId id);

typedef Ids::IdAllocator<VkCommandBuffer, VkCommandPool> VkCommandBufferAllocator;
extern CommandBufferPools pools;

} // Vulkan
