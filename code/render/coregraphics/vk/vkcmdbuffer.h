#pragma once
//------------------------------------------------------------------------------
/**
	Implements a Vulkan command buffer

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "vulkan/vulkan.h"
#include "coregraphics/cmdbuffer.h"

namespace Vulkan
{

static const uint NumPoolTypes = 4;
struct CommandBufferPools
{

	VkCommandPool pools[CoreGraphics::NumCmdBufferUsages][NumPoolTypes];
	uint queueFamilies[CoreGraphics::NumCmdBufferUsages];
	VkDevice dev;
};

/// setup pools for a certain device
void SetupVkPools(VkDevice dev, uint32_t drawQueue, uint32_t computeQueue, uint32_t transferQueue, uint32_t sparseQueue);
/// destroy pools
void DestroyVkPools(VkDevice dev);

/// get pool based on flags
const VkCommandPool CommandBufferGetVkPool(CoreGraphics::CmdBufferUsage usage, VkCommandPoolCreateFlags flags);

/// get vk command buffer
const VkCommandBuffer CommandBufferGetVk(const CoreGraphics::CmdBufferId id);

typedef Ids::IdAllocator<VkCommandBuffer, VkCommandPool> VkCommandBufferAllocator;
extern CommandBufferPools pools;

} // Vulkan
