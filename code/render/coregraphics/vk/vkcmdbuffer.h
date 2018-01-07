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
	// 0 is main, 1 is secondary
	VkCommandPool draw[NumPoolTypes];
	VkCommandPool transfer[NumPoolTypes];
	VkCommandPool compute[NumPoolTypes];
	VkCommandPool sparse[NumPoolTypes];

	VkDevice dev;
};

/// setup pools for a certain device
void SetupVkPools(VkDevice dev, uint32_t drawQueue, uint32_t computeQueue, uint32_t transferQueue, uint32_t sparseQueue);
/// destroy pools
void DestroyVkPools(VkDevice dev);

/// get vk command buffer
const VkCommandBuffer CommandBufferGetVk(const CoreGraphics::CmdBufferId id);

typedef Ids::IdAllocator<VkCommandBuffer, VkCommandPool> VkCommandBufferAllocator;
extern CommandBufferPools pools;

} // Vulkan
