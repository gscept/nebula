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

static const SizeT EventMaxNumBarriers = 16;

struct VkEventInfo
{
	VkEvent event;
	VkPipelineStageFlags leftDependency;
	VkPipelineStageFlags rightDependency;
	VkImageMemoryBarrier imageBarriers[EventMaxNumBarriers];
	uint32_t numImageBarriers;
	VkBufferMemoryBarrier bufferBarriers[EventMaxNumBarriers];
	uint32_t numBufferBarriers;
	VkMemoryBarrier memoryBarriers[EventMaxNumBarriers];
	uint32_t numMemoryBarriers;
	
};
typedef Ids::IdAllocator<VkDevice, VkEventInfo> VkEventAllocator;
extern VkEventAllocator eventAllocator;
} // namespace Vulkan
