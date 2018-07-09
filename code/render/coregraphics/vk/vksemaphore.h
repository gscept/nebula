#pragma once
//------------------------------------------------------------------------------
/**
	Vulkan semaphore (cross-queue synchronization primitive)

	(C) 2018 Individual contributors, see AUTHORS file
*/
//------------------------------------------------------------------------------
#include "coregraphics/semaphore.h"
#include "ids/idallocator.h"
#include <vulkan/vulkan.h>

namespace Vulkan
{

typedef Ids::IdAllocator<
	VkDevice,
	VkSemaphore
> VkSemaphoreAllocator;
extern VkSemaphoreAllocator semaphoreAllocator;

/// get vulkan sampler
const VkSemaphore& SemaphoreGetVk(const CoreGraphics::SemaphoreId& id);

} // namespace Vulkan
