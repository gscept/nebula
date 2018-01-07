#pragma once
//------------------------------------------------------------------------------
/**
	The Vulkan implementation of a GPU command barrier

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "coregraphics/barrier.h"

namespace Vulkan
{

static const SizeT MaxNumBarriers = 16;

struct VkBarrierInfo
{
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

typedef Ids::IdAllocator<VkBarrierInfo> VkBarrierAllocator;
extern VkBarrierAllocator barrierAllocator;
} // namespace Vulkan
