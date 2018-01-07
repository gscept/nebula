#pragma once
//------------------------------------------------------------------------------
/**
	Vulkan abstraction of event

	(C) 2017 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "ids/idallocator.h"
#include "vulkan/vulkan.h"
namespace Vulkan
{

static const SizeT MaxNumBarriers = 16;

struct VkEventInfo
{
	VkEvent event;
	VkPipelineStageFlags leftDependency;
	VkPipelineStageFlags rightDependency;
	VkImageMemoryBarrier imageBarriers[MaxNumBarriers];
	uint32_t numImageBarriers;
	VkBufferMemoryBarrier bufferBarriers[MaxNumBarriers];
	uint32_t numBufferBarriers;
	VkMemoryBarrier memoryBarriers[MaxNumBarriers];
	uint32_t numMemoryBarriers;
	
};
typedef Ids::IdAllocator<VkDevice, VkEventInfo> VkEventAllocator;
} // namespace Vulkan
